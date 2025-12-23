#include "SimpleShader.hlsli"

float4 pixel(VSOutput a_input) : SV_Target
{
	float4 _outColor = {0.0f,0.0f,0.0f,0.0f };
	
	_outColor = _MainTex.Sample(smp, a_input.uv) * baseColor;

	return _outColor;
}
