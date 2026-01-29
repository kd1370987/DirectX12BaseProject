#include "QuadRenderingShader.hlsli"

float4 ps(Output a_input) : SV_TARGET
{
	float4 _texColor = g_tex.Sample(g_smp,a_input.uv);
	float4 _result = _texColor;

	float _gray = dot(_result.rgb, float3(0.299,0.587,0.114));
	_result.rgb = _gray;
	
	return _result;
}
