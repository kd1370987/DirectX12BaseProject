
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

	// 過去UVを逆算
	float2 _prevUV = _uv - _velocity;

	// 過去UVの画面外チェック
	if (_prevUV.x < 0.0f || _prevUV.x > 1.0f || _prevUV.y < 0.0f || _prevUV.y > 1.0f)
	{
		// 画面外から入ってきた新しいピクセルは過去の履歴がないのでそのまま出力
		g_outputTAA[_centerCoord] = _currentColor;
		return;
	}

	// 過去の履歴をバイリニアサンプリング
	float4 _historyColor = g_historyColorTex.SampleLevel(g_smp, _prevUV, 0);

	// -------------------------------------------------------------------------------
	// 3x3の近傍ピクセルをループ
	// min , max を取得してクランプする

	float3 _minColor = float3(1e10f, 1e10f, 1e10f);
	float3 _maxColor = float3(-1e10f, -1e10f, -1e10f);

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

			// 比較
			_minColor = min(_minColor, _sampleColor);
			_maxColor = max(_maxColor, _sampleColor);
		}
	}

	// 過去の色が箱に収まっていないのならクランプ（ゴースト対策）
	float3 _historyColor3 = _historyColor.rgb;
	_historyColor3 = clamp(
		_historyColor3,
		_minColor,
		_maxColor
	);

	// 過去の色と現在の色をブレンド（現在の割合が 10%）
	float3 _outColor = lerp(_historyColor3, float3(_currentColor.rgb), 0.1f);

	// 出力
	g_outputTAA[_centerCoord] = float4(_outColor,1);
}
