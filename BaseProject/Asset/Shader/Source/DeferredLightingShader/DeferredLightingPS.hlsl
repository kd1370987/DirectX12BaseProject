#include "DeferredLightingShader.hlsli"

float3 ReconstructViewPos(float2 uv, float depth)
{
	float4 clip = float4(uv * 2 - 1, depth, 1);
	float4 view = mul(cProjInv, clip);
	return view.xyz / view.w;
}

float4 ps(VSOutput i) : SV_Target
{
	//float d = g_depthTex.Sample(g_samp, i.uv).r;
	//return float4(d, d, d, 1);

	//return float4(g_albedoTex.Sample(g_samp, i.uv).rgb, 1);

	float3 albedo = g_albedoTex.Sample(g_samp, i.uv).rgb;
	float3 normal = g_normalTex.Sample(g_samp, i.uv).xyz * 2 - 1;
	float depth = g_depthTex.Sample(g_samp, i.uv).r;

	float3 viewPos = ReconstructViewPos(i.uv, depth);

    // 仮ライト
	float3 L = normalize(float3(1, 1, -1));
	float NdotL = saturate(dot(normal, L));

	float3 color = albedo * NdotL;

	return float4(color, 1);
}
