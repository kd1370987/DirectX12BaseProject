#include "../../../Source/CalcNormal.hlsli"
#include "../../../Source/RootSignatureLayout.hlsli"

// ルートシグネチャデータ
#define GISPATIALDENOISE_ROOT_SIG \
"RootFlags(0), " \
"CBV(b0)," \
"DescriptorTable(SRV(t0, numDescriptors=3)),"\
"DescriptorTable(UAV(u0, numDescriptors=1)),"\
RS_STATIC_SAMPLER

// デノイズ設定
struct DenoiseSettings
{
	int stepSize;		// パスごとのステップサイズ
	float phiDepth;	// 深度の感度（小さいほどエッジを厳密に保護）
	float phiNormal;	// 法線の感度（大きいほど法線のずれに敏感）
	float phiColor;	// 輝度の感度（ノイズとディティールの境界制御）
};
cbuffer CBDenoiseSettings : register(b0)
{
	DenoiseSettings g_denoiseSettings;
}

// 入力
Texture2D<float4> g_GITex : register(t0);	// 時間デノイズされたGI
Texture2D<float4> g_depthTex : register(t1);		// 現在深度
Texture2D<float4> g_normalTex : register(t2);		// 現在法線

// 出力
RWTexture2D<float4> g_outputGI : register(u0); // 結果書き込み用

// サンプラー
SamplerState g_smp : register(s0);

// 5x5 A-Trous カーネル用のB3スプライン重み（1次元）
// 二次元展開したときの中心(0)が 6/16, 前後が 4/16,1/16
static const float g_kernel[5] = {
	1.0f / 16.0f,
	4.0f / 16.0f,
	6.0f / 16.0f,
	4.0f / 16.0f,
	1.0f / 16.0f
};


// ルートシグネチャセット
[RootSignature(GISPATIALDENOISE_ROOT_SIG)]

[numthreads(8, 8, 1)]
void CSMain( uint3 DTid : SV_DispatchThreadID )
{
	// 画面サイズを取得
	uint _width, _height;
	g_outputGI.GetDimensions(_width,_height);

	// 画面外スレッドの早期抜出
	if (DTid.x >= _width || DTid.y >= _height) return;

	int2 _centerCoord = int2(DTid.xy);

	// 注目画素（中心）の情報を取得
	float4 _centerColor = g_GITex.Load(int3(_centerCoord,0));
	float _centerDepth = g_depthTex.Load(int3(_centerCoord,0)).r;
	float3 _centerNormal = DecsodeNormal(g_normalTex.Load(int3(_centerCoord,0)).xy);

	// 法線を[-1, 1]に展開
	//_centerNormal = _centerNormal * 2.0f - 1.0f;

	float4 _sumColor = float4(0.0f, 0.0f, 0.0f, 0.0f);
	float _sumWeight = 0.0f;

	// 5x5の近傍ピクセルをループ
	// A-Trousアルゴリズムを使う 
	// 定数バッファでもらい受けたステップサイズに応じて少しずつ離れた場所をサンプリングする
	[unroll]
	for (int _y = -2; _y <= 2; ++_y)
	{
		[unroll]
		for (int _x = -2; _x <= 2; ++_x)
		{
			// サンプリング座標を計算（ステップサイズに応じて外側に広がる）
			int2 _sampleCoord = _centerCoord + int2(_x, _y) * g_denoiseSettings.stepSize;

			// 画面クランプ
			_sampleCoord = clamp(_sampleCoord, int2(0, 0), int2(_width - 1, _height - 1));

			// サンプル画素の情報を取得
			float4 _sampleColor	 = g_GITex.Load(int3(_sampleCoord,0));
			float  _sampleDepth	 = g_depthTex.Load(int3(_sampleCoord,0)).r;
			float3 _sampleNormal = DecsodeNormal(g_normalTex.Load(int3(_sampleCoord, 0)).xy);
			//_sampleNormal = _sampleNormal * 2.0f - 1.0f;
			
			// ---- ベースとなるフィルターの重み : B3 Spline ----
			float _filterWeight = g_kernel[_x + 2] * g_kernel[_y + 2];

			// ---- 深度エッジウェイト計算 ----
			// 深度の差が等精度の平面からどれだけ離れているかを評価
			float _depthDiff = abs(_centerDepth - _sampleDepth);
			float _wDepth = exp(-_depthDiff / max(g_denoiseSettings.phiDepth, 1e-5f));

			// ---- 法線エッジウェイトの計算 ----
			// 内積(cosΘ)をベースに、角度のずれが急かどうか評価
			float _normalDot = max(0.0f, dot(_centerNormal, _sampleNormal));
			float _wNormal = pow(_normalDot, g_denoiseSettings.phiNormal);

			// ---- カラーウェイトの計算（輝度計算 : オプショナル） ----
			// 将来的にはSVGFではここに分散を絡める。
			// 現在は輝度差ベースで実装
			float3 _colorDiff = _centerColor.rgb - _sampleColor.rgb;
			float _distColorSq = dot(_colorDiff,_colorDiff);
			float _wColor = exp(-_distColorSq / max(g_denoiseSettings.phiColor, 1e-5f));

			// すべての重みを乗算
			float _finalWeight = _filterWeight * _wDepth * _wNormal * _wColor;
			//float _finalWeight = _filterWeight;

			// ブレンド計算
			_sumColor += _sampleColor * _finalWeight;
			_sumWeight += _finalWeight;
		}
	}

	// 重みの合計で正規化して出力（ゼロ除算対策）
	if (_sumWeight > 0.0f)
	{
		g_outputGI[ _centerCoord] = _sumColor / _sumWeight;
	}
	else
	{
		g_outputGI[_centerCoord] = _centerColor;
	}
}
