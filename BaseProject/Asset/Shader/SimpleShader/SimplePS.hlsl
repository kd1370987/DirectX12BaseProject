#include "SimpleShader.hlsli"

float4 pixel(VSOutput a_input) : SV_Target
{
	float4 _baseColor = baseColor;
	_baseColor.rgb *= a_input.color.rgb; // 頂点色を乗算
	//return _MainTex.Sample(smp, a_input.uv);
	return float4(baseColor);
	//return testColor;
}
