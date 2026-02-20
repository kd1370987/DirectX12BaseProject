#include "QuadRenderingShader.hlsli"

float3 ACESFilm(float3 x)
{
	float a = 2.51;
	float b = 0.03;
	float c = 2.43;
	float d = 0.59;
	float e = 0.14;
	return saturate((x * (a * x + b)) / (x * (c * x + d) + e));
}

float4 ps(Output a_input) : SV_TARGET
{
	float4 _texColor = g_tex.Sample(g_smp,a_input.uv);
	float4 _result = _texColor;

	// ACESフィルムライクトーンマッピングを適用
	_result.rgb = ACESFilm(_result.rgb);
	
	return _result;
}
