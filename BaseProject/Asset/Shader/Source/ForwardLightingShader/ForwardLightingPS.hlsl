#include "ForwardLightingShader.hlsli"

#include "../CalcLighting.hlsli"

float4 ps(VSOutput a_input) : SV_Target
{
	// テクスチャから情報を取得
	float4 _albedo = g_mainTex.Sample(g_samp,a_input.uv).rgba;		// ベースカラー
	float3 _normalTex = g_normalTex.Sample(g_samp,a_input.uv).rgb * 2 -1;
	float3x3 TBN =
	{
		normalize(a_input.wT),
		normalize(a_input.wB),
		normalize(a_input.wN)
	};
	float3 _normal = mul(_normalTex, TBN);							// 法線
	float3 _material = g_normalTex.Sample(g_samp,a_input.uv).xyz;
	float _roughness = _material.y;									// 滑らかさ
	float _metallic = _material.x;									// 金属度
	float4 _emissive = g_emiTex.Sample(g_samp,a_input.uv);			// エミッシブ
	
	float4 _outColor = {1.0f,0.0f,1.0f,1.0f };

	// 平行光
	//float3 _L = normalize(-g_DL_Dir);
	
	_outColor = _albedo;

	return _outColor;
}
