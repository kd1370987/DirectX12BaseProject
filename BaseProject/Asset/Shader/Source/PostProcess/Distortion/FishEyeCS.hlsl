#include "../../../Common/RootSignatureLayout.hlsli"

struct FishEyeOption
{
	float strength;		// 歪みの強さ
	float2 center;		// 中心点, 0.5 ,0.5
	
	float pad;
};
cbuffer CBFishEye : register(b1)
{
	FishEyeOption g_option;
}

// 入力
Texture2D<float4> g_inputTex : register(t0);

// 出力
RWTexture2D<float4> g_outTex : register(u0);

// サンプラー
SamplerState g_samp : register(s0);

[numthreads(8, 8, 1)]
void CSMain( uint3 DTid : SV_DispatchThreadID )
{
	// 出力画像の解像度を取得
	uint _width, _height;
	g_outTex.GetDimensions(_width,_height);
	
	// 画面外チェック
	if (DTid.x >= _width || DTid.y >= _height) return;
	
	float2 _uv = (DTid.xy + 0.5f) / float2(_width,_height);
	float2 _direction = _uv - g_option.center;

	
	// 距離計算だけアスペクト比補正
	float _aspect = (float) _width / (float) _height;
	_direction.x *= _aspect;

	// 中心からの距離
	float _r = length(_direction);

	// 魚眼の歪み
	float _scale = 1.0 + g_option.strength * _r * _r;

	// 歪みのUV
	float2 _distortedUV = g_option.center + _direction * _scale;

	// 範囲外処理
	if (any(_distortedUV < 0.0f) || any(_distortedUV > 1.0f))
	{
		g_outTex[DTid.xy] = float4(0, 0, 0, 1);
		return;
	}

	// 出力
	g_outTex[DTid.xy] = g_inputTex.SampleLevel(g_samp, _distortedUV, 0);
}
