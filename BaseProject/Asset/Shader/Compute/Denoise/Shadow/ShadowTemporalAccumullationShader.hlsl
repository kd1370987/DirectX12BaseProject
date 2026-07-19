
#include "../../../Source/CalcNormal.hlsli"
#include "../../../Source/RootSignatureLayout.hlsli"

// ルートシグネチャデータ
#define SHADOW_TEMPORALACCUMULATION_ROOT_SIG \
"RootFlags(0), " \
"CBV(b0),"\
"DescriptorTable(SRV(t0, numDescriptors=7)),"\
"DescriptorTable(UAV(u0, numDescriptors=1)),"\
RS_STATIC_SAMPLER

// ブレンド比率などのオプション
struct ShadowTAOption
{
	float phiDepth;
	float phiNormal;
	float blendRate;
};
cbuffer CBShadowTAOption : register(b0)
{
	ShadowTAOption g_option;
}

// 入力
Texture2D<float4> g_currentShadowTex : register(t0); // 現在のGI
Texture2D<float4> g_velocityTex : register(t1); // モーションベクター
Texture2D<float4> g_historyShadowTex : register(t2); // 前フレームのGI
Texture2D<float4> g_depthTex : register(t3); // 現在深度
Texture2D<float4> g_normalTex : register(t4); // 現在法線
Texture2D<float4> g_prevDepthTex : register(t5); // 過去深度
Texture2D<float4> g_prevNormalTex : register(t6); // 過去法線

// 出力
RWTexture2D<float4> g_outputShadow : register(u0); // 結果書き込み用

// サンプラー
SamplerState g_smp : register(s0);

// ルートシグネチャセット
[RootSignature(SHADOW_TEMPORALACCUMULATION_ROOT_SIG)]


[numthreads(8, 8, 1)]
void CSMain( uint3 DTid : SV_DispatchThreadID )
{
	// 画像の解像度を取得
	uint _width, _height;
	g_outputShadow.GetDimensions(_width,_height);

	// ピクセル座標が画面内かどうかチェック
	int2 _centerCoord = int2(DTid.xy);
	if(_centerCoord.x >= _width || _centerCoord.y >= _height) return;

	// UV座標の計算
	float2 _uv = (_centerCoord.xy + 0.5f) / float2(_width,_height);

	// 現在の情報を取得
	int3 _location = int3(_centerCoord,0);
	float4 _currentShadow = g_currentShadowTex.Load(_location);
	float3 _currentNormal = DecsodeNormal(g_normalTex.Load(_location).rg);
	float _currentDepth = g_depthTex.Load(_location).r;

	// -------------------------------------------------------------------------------
	// 3x3の近傍ピクセルをループ
	// 平均(m1)と二乗平均(m2)から分散を求めて統計的にクランプ範囲を決める(Variance Clipping)
	// 同時に一番手前のピクセルを探して Velocity Dilation に使う

	float3 _m1 = float3(0, 0, 0);	// 色の合計
	float3 _m2 = float3(0, 0, 0);	// 色の二乗の合計

	// Velocity Dilation用（一番手前のピクセルのモーションベクターを採用する）
	float _closestDepth = 1e10f;
	int2 _closestCoord = _centerCoord;

	[unroll]
	for (int _y = -1; _y <= 1; ++_y)
	{
		[unroll]
		for (int _x = -1; _x <= 1; ++_x)
		{
			// サンプリング座標を取得
			int2 _sampleCoord = _centerCoord + int2(_x, _y);
			_sampleCoord = clamp(_sampleCoord, int2(0, 0), int2(_width - 1, _height - 1)); // 画面クランプ

			// サンプル画素を取得
			float3 color = g_currentShadowTex.Load(int3(_sampleCoord, 0)).rgb;
			_m1 += color;
			_m2 += color * color;

			// もっと手前のピクセルを探す(Velocity Dilation)
			float _d = g_depthTex.Load(int3(_sampleCoord, 0)).r;
			if (_d < _closestDepth)
			{
				_closestDepth = _d;
				_closestCoord = _sampleCoord;
			}
		}
	}

	// 平均(mu)と分散から標準偏差(sigma)を求める
	float3 _mu = _m1 / 9.0f;
	float3 _sigma = sqrt(abs(_m2 / 9.0f - _mu * _mu));

	// 係数ガンマ（小さいほどゴーストは減るがボケやすくなる）
	const float _gamma = 1.0f;
	float3 _minColor = _mu - _gamma * _sigma;
	float3 _maxColor = _mu + _gamma * _sigma;

	// モーションベクターから過去UVを取得
	// 中心ではなく一番手前にあるピクセルのVelocityを使って過去UVを計算する
	float2 _velocity = g_velocityTex.Load(int3(_closestCoord, 0)).xy;
	float2 _prevUV = _uv - _velocity;

	// 過去UV画面外チェック
	// 画面外チェック
	if (_prevUV.x < 0.0f || _prevUV.x > 1.0f || _prevUV.y < 0.0f || _prevUV.y > 1.0f)
	{
		// 画面外から入ってきた新しいピクセルは過去の履歴がないのでそのまま出力
		g_outputShadow[DTid.xy] = _currentShadow;
		return;
	}

	// 過去の履歴をバイリニアサンプリング
	float4 _historyShadow = g_historyShadowTex.SampleLevel(g_smp, _prevUV, 0);
	float3 _prevNormal = DecsodeNormal(g_prevNormalTex.SampleLevel(g_smp, _prevUV, 0).rg);
	float _prevDepth = g_prevDepthTex.SampleLevel(g_smp, _prevUV, 0).r;

	// 統計的なクランプで履歴を現在の分布内に収める(Variance Clipping)
	_historyShadow.rgb = clamp(_historyShadow.rgb, _minColor, _maxColor);

	// -------------------------------------------------------------------------------
	// ゴースト対策（ディスオクルージョン判定）
	// これまで隠れていた背景や物体が新たに露出する現象を検出する
	bool _isValidHistory = true;

	// 法線の向きが違いすぎる場合は履歴を捨てる
	if (dot(_currentNormal, _prevNormal) < g_option.phiNormal)
	{
		_isValidHistory = false;
	}

	// 深度が違いすぎる場合は履歴を捨てる
	// いったん深度値で比較。ワールド座標での比較に変更予定
	if (abs(_currentDepth - _prevDepth) > g_option.phiDepth)
	{
		_isValidHistory = false;
	}

	// -------------------------------------------------------------------------------
	// ブレンド処理
	float4 _finalShadow = _currentShadow;

	if (_isValidHistory)
	{
		// 履歴が有効ならブレンド
		// アルファが小さいほどノイズは消えるが、動いた時の残像が出やすくなる
		float _alpha = g_option.blendRate;
		_finalShadow = lerp(_historyShadow, _currentShadow, _alpha);
	}

	// 結果を出力（RGBに色、AにHitDistance）
	g_outputShadow[DTid.xy] = _finalShadow;
}
