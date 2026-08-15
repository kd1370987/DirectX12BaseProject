//==========================================================================================
//
// GaussianBlurShader
//
// 入力テクスチャにガウシアンブラーを掛けて、出力テクスチャの解像度で書き出す汎用ブラー。
// 入力と出力の解像度が違っていてよいので、これ1本で縮小も拡大も兼ねる。
//
//   縮小 : 出力(1/2)の画素から、入力(等倍)を srcTexelSize 刻みでサンプリングする
//          → 単純な間引きではなく、周囲を含めて平均した縮小になる
//   拡大 : 出力(等倍)の画素から、入力(1/16など)を srcTexelSize 刻みでサンプリングする
//          → バイリニア補間で引き伸ばしつつ、さらにボカされる
//
// サンプル位置は「入力テクセル単位」で刻むのがポイント。出力側の刻みにすると
// 拡大時にオフセットが入力の1テクセル未満になり、ブラーが効かなくなる。
//
// 重みは分離可能なガウス分布 exp(-(x^2+y^2) / 2σ^2) をそのまま2次元で回して正規化する。
// タップ数は (tapRadius*2+1)^2 なので、拡大側（フル解像度）では tapRadius を小さくして渡すこと。
//
//==========================================================================================
#include "../../../Source/RootSignatureLayout.hlsli"

// ルートシグネチャデータ
#define GAUSSIAN_BLUR_RS \
"RootFlags(0)," \
"CBV(b0)," \
"DescriptorTable(SRV(t0, numDescriptors=1)), " \
"DescriptorTable(UAV(u0, numDescriptors=1)), " \
RS_STATIC_SAMPLER_CLAMP

// 入力
Texture2D<float4> g_srcTex : register(t0);

// 出力
RWTexture2D<float4> g_outTex : register(u0);

// サンプラー : 端をクランプする（WRAPだと画面外で反対側の色が回り込む）
SamplerState g_samp : register(s0);

// C++ 側（GaussianBlurPass）から渡されるブラー設定
struct GaussianBlurSetting
{
	float2 srcTexelSize;	// 入力テクスチャの1テクセルぶんのUV（= 1 / 入力解像度）
	float  sigma;			// ガウス分布の標準偏差（入力テクセル単位）
	int    tapRadius;		// 片側のタップ数（0でブラーなし）
};
cbuffer GaussianBlurSettingCB : register(b0)
{
	GaussianBlurSetting g_blur;
}

// 1パスあたりのタップ数の上限。CB側の値が壊れても暴走しないよう必ずクランプする
#define GAUSSIAN_MAX_RADIUS 4

[RootSignature(GAUSSIAN_BLUR_RS)]
[numthreads(8, 8, 1)]
void CSMain(uint3 DTid : SV_DispatchThreadID)
{
	// 出力の解像度を取得（ディスパッチ数は切り上げなので末尾は範囲外になりうる）
	uint _width, _height;
	g_outTex.GetDimensions(_width, _height);

	// 画面外ならリターン
	if (DTid.x >= _width || DTid.y >= _height) return;

	// 出力画素の中心に対応する入力のUV
	float2 _uv = (DTid.xy + 0.5f) / float2(_width, _height);

	int _radius = clamp(g_blur.tapRadius, 0, GAUSSIAN_MAX_RADIUS);

	// ブラー無しならそのまま拾って終わり
	if (_radius == 0)
	{
		g_outTex[DTid.xy] = float4(g_srcTex.SampleLevel(g_samp, _uv, 0).rgb, 1.0f);
		return;
	}

	// exp(-(x^2+y^2) / 2σ^2) の分母。σが0だと0除算になるので下限を入れる
	float _sigma = max(g_blur.sigma, 1e-4f);
	float _denom = 2.0f * _sigma * _sigma;

	float3 _sum = 0.0f;
	float _weightSum = 0.0f;

	[loop]
	for (int _y = -_radius; _y <= _radius; ++_y)
	{
		[loop]
		for (int _x = -_radius; _x <= _radius; ++_x)
		{
			float2 _offset = float2(_x, _y);
			float _weight = exp(-dot(_offset, _offset) / _denom);

			// 入力テクセル単位でずらす。サンプラーがCLAMPなので端は自動で止まる
			float2 _sampleUV = _uv + _offset * g_blur.srcTexelSize;

			_sum += g_srcTex.SampleLevel(g_samp, _sampleUV, 0).rgb * _weight;
			_weightSum += _weight;
		}
	}

	g_outTex[DTid.xy] = float4(_sum / _weightSum, 1.0f);
}
