#include "SimpleShader.hlsli"

float4 pixel(VSOutput a_input) : SV_Target
{
	return _MainTex.Sample(smp, a_input.uv);
}
