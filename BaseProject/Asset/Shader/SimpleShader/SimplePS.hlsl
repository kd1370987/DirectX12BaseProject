#include "SimpleShader.hlsli"

float4 pixel(VSOutput a_input) : SV_Target
{
	//return float4(1,0,0,1);

	//return baseColor;
	
	float4 _outColor = {0.0f,0.0f,0.0f,0.0f };
	
	//_outColor = _MainTex.Sample(smp, a_input.uv) * baseColor;
	_outColor = _MainTex.Sample(smp, a_input.uv) * a_input.color;

	return _outColor;
}
