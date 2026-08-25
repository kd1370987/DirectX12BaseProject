#include "../../../../Common/Math/CalcNormal.hlsli"
#include "../../../../Common/RootSignatureLayout.hlsli"
#include "../../../../Common/RootParameters/DenoiseSetting.hlsli"

//==========================================================================================
// ルートパラメーター
//
//   0 : CBV(b0)            デノイズ設定(A-Trous のステップごとに書き換える)
//   1 : SRVテーブル(t0-t2) 影 + 深度 + 法線(すべてフル解像度)
//   2 : UAVテーブル(u0)    出力影マスク
//==========================================================================================
#define SHADOWSPATIALDENOISE_ROOT_SIG \
"RootFlags(0), " \
"CBV(b0)," \
"DescriptorTable(SRV(t0, numDescriptors=3)),"\
"DescriptorTable(UAV(u0, numDescriptors=1)),"\
RS_STATIC_SAMPLER

cbuffer CBDenoiseSettings : register(b0)
{
	SpatialDenoiseSetting g_denoiseSettings;
}

// 入力
Texture2D<float4> g_shadowTex : register(t0);	// 時間デノイズ済みの影(フル解像度)
Texture2D<float4> g_depthTex  : register(t1);	// 現在深度(フル解像度)
Texture2D<float4> g_normalTex : register(t2);	// 現在法線(フル解像度)

// 出力
RWTexture2D<float4> g_outputShadow : register(u0); // 結果書き込み用

// サンプラー
SamplerState g_smp : register(s0);

// 5x5 A-Trous カーネル用のB3スプライン重み（1次元）
static const float g_kernel[5] = {
	1.0f / 16.0f,
	4.0f / 16.0f,
	6.0f / 16.0f,
	4.0f / 16.0f,
	1.0f / 16.0f
};

// ルートシグネチャセット
[RootSignature(SHADOWSPATIALDENOISE_ROOT_SIG)]

[numthreads(8, 8, 1)]
void CSMain( uint3 DTid : SV_DispatchThreadID )
{
	// 画面サイズを取得
	uint _width, _height;
	g_outputShadow.GetDimensions(_width, _height);

	// 画面外スレッドの早期抜出
	if (DTid.x >= _width || DTid.y >= _height) return;

	int2 _centerCoord = int2(DTid.xy);

	// 注目画素（中心）の情報を取得
	// 影はGBufferと同解像度なので、GIのスペースデノイズと違い座標のスケールは不要
	float4 _centerShadow = g_shadowTex.Load(int3(_centerCoord, 0));
	float  _centerDepth  = g_depthTex.Load(int3(_centerCoord, 0)).r;
	float3 _centerNormal = DecsodeNormal(g_normalTex.Load(int3(_centerCoord, 0)).xy);

	float4 _sumColor  = float4(0.0f, 0.0f, 0.0f, 0.0f);
	float  _sumWeight = 0.0f;

	// 5x5の近傍ピクセルをループ（A-Trous : ステップサイズに応じて外側へ広がる）
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
			float4 _sampleShadow = g_shadowTex.Load(int3(_sampleCoord, 0));
			float  _sampleDepth  = g_depthTex.Load(int3(_sampleCoord, 0)).r;
			float3 _sampleNormal = DecsodeNormal(g_normalTex.Load(int3(_sampleCoord, 0)).xy);

			// ---- ベースとなるフィルターの重み : B3 Spline ----
			float _filterWeight = g_kernel[_x + 2] * g_kernel[_y + 2];

			// ---- 深度エッジウェイト計算 ----
			float _depthDiff = abs(_centerDepth - _sampleDepth);
			float _wDepth = exp(-_depthDiff / max(g_denoiseSettings.phiDepth, 1e-5f));

			// ---- 法線エッジウェイトの計算 ----
			float _normalDot = max(0.0f, dot(_centerNormal, _sampleNormal));
			float _wNormal = pow(_normalDot, g_denoiseSettings.phiNormal);

			// ---- カラー(影の濃さ)ウェイトの計算 ----
			float3 _colorDiff = _centerShadow.rgb - _sampleShadow.rgb;
			float _distColorSq = dot(_colorDiff, _colorDiff);
			float _wColor = exp(-_distColorSq / max(g_denoiseSettings.phiColor, 1e-5f));

			// すべての重みを乗算
			float _finalWeight = _filterWeight * _wDepth * _wNormal * _wColor;

			// ブレンド計算
			_sumColor  += _sampleShadow * _finalWeight;
			_sumWeight += _finalWeight;
		}
	}

	// 重みの合計で正規化して出力（ゼロ除算対策）
	if (_sumWeight > 0.0f)
	{
		g_outputShadow[_centerCoord] = _sumColor / _sumWeight;
	}
	else
	{
		g_outputShadow[_centerCoord] = _centerShadow;
	}
}
