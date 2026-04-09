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
	float4 _result = float4(0, 0, 0, 1);
	
	float4 _texColor = g_tex.Sample(g_smp,a_input.uv);
	// ACESフィルムライクトーンマッピングを適用
	_texColor.rgb = ACESFilm(_texColor.rgb);
	
	float4 _ui = g_ui.Sample(g_smp,a_input.uv);

	_result.rgb = _ui.rgb + _texColor.rgb * (1.0 - _ui.a);
	_result.a = 1.0;
	
	return _result;
}
