#include "SimpleShader.hlsli"

float4 pixel(VSOutput a_input) : SV_Target
{
	float4 base = g_mainTex.Sample(smp, a_input.uv) * baseColor;

	float3 lightDir = normalize(float3(1, -1, 1));

	float3 nTS = g_normalTex.Sample(smp, a_input.uv).xyz * 2.0f - 1.0f;

	float3 T = normalize(a_input.wT);
	float3 B = normalize(a_input.wB);
	float3 N = normalize(a_input.wN);

	float3 nWS = normalize(
        T * nTS.x +
        B * nTS.y +
        N * nTS.z
    );

	float brightness = saturate(dot(-lightDir, nWS));

	return base * brightness;
}
