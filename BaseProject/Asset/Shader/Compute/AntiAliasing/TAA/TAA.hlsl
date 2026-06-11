
#include "../../../Source/CalcNormal.hlsli"
#include "../../../Source/RootSignatureLayout.hlsli"

// ルートシグネチャデータ
#define TEMPORALACCUMULATION_ROOT_SIG \
"RootFlags(0), " \
"DescriptorTable(SRV(t0, numDescriptors=5)),"\
"DescriptorTable(UAV(u0, numDescriptors=1)),"\
RS_STATIC_SAMPLER

// 入力
Texture2D<float4> g_currentColorTex : register(t0); // 現在の色
Texture2D<float4> g_historyColorTex : register(t1); // 過去の色
Texture2D<float2> g_motionVectorTex : register(t2); // モーションベクター
Texture2D<float1> g_depthTex		: register(t3); // 現在深度
Texture2D<float2> g_normalTex		: register(t4); // 現在法線

// 出力
RWTexture2D<float4> g_outputTAA : register(u0); // 結果書き込み用
// サンプラー
SamplerState g_smp : register(s0);

// ルートシグネチャセット
[RootSignature(TEMPORALACCUMULATION_ROOT_SIG)]

[numthreads(8, 8, 1)]
void CSMain( uint3 DTid : SV_DispatchThreadID )
{
	// 画面サイズの取得
	float _width, _height;
	g_outputTAA.GetDimensions(_width, _height);

	// 画面外の数レッドなら早期リターン
	if (DTid.x >= _width || DTid.y >= _height) return;

	// ピクセル計算
	int2 _centerCoord = int2(DTid.xy);							// センター座標
	float2 _uv = (DTid.xy + 0.5f) / float2(_width, _height);	// UV座標
	

	// 注目画素（中心）の情報を取得
	int3 _location = int3(_centerCoord,0);
	float4 _currentColor = g_currentColorTex.Load(_location).rgba;
	float2 _velocity = g_motionVectorTex.Load(_location).rg;
	float _depth = g_depthTex.Load(_location).r;
	float3 _normal = DecsodeNormal(g_normalTex.Load(_location).rg);
	
	// -------------------------------------------------------------------------------
	// 3x3の近傍ピクセルをループ
	// min , max を取得してクランプする
	// Velocity Dilation と Variance Clipping
	
	// 統計用変数
	float3 _m1 = float3(0, 0, 0);		// 色の合計
	float3 _m2 = float3(0, 0, 0);

	// Velocity Dilation用変数
	float _closestDepth = 1.0f;
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
			float3 _sampleColor = g_currentColorTex.Load(int3(_sampleCoord,0)).rgb;
			_m1 += _sampleColor;
			_m2 += _sampleColor * _sampleColor;

			// もっと手前のピクセルを探す(Velocity Dilation)
			float _d = g_depthTex.Load(int3(_sampleCoord, 0)).r;
			if(_d < _closestDepth)
			{
				_closestDepth = _d;
				_closestCoord = _sampleCoord;
			}
		}
	}

	// 平均(mu)と分散から標準偏差(sigma)を求める
	float3 _mu = _m1 / 9.0f;
	float3 _sigma = sqrt(abs(_m2 / 9.0f - _mu * _mu));

	// 係数ガンマ
	float _gamma = 0.5f;	// 1.0 ～ 1.25 あたりがゴーストとボケのバランスがいい
	float3 _minColor = _mu - _gamma * _sigma;
	float3 _maxColor = _mu + _gamma * _sigma;

	// 中心ではなく一番手前にあるピクセルのVelocityを使って過去UVを計算
	float2 _closestVelocity = g_motionVectorTex.Load(int3(_closestCoord,0)).rg;
	float2 _prevUV = _uv - _closestVelocity;

	if (_prevUV.x < 0.0f || _prevUV.x > 1.0f || _prevUV.y < 0.0f || _prevUV.y > 1.0f)
	{
		g_outputTAA[_centerCoord] = _currentColor;
		return;
	}
	
	// 過去カラーを取得
	float4 _historyColor = g_historyColorTex.SampleLevel(g_smp, _prevUV, 0);

	// クランプ
	float3 _historyColor3 = clamp(_historyColor.rgb, _minColor, _maxColor);

	// ブレンド
	float3 _outColor = lerp(_historyColor3, float3(_currentColor.rgb), 0.1f);

	// 出力
	g_outputTAA[_centerCoord] = float4(_outColor, 1);
}
