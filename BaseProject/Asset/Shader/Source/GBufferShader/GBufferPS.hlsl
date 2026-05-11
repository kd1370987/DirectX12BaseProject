#include "GBufferShader.hlsli"

#include "../CalcNormal.hlsli"


// HLSL側の記述例
[RootSignature(GBUFFER_ROOT_SIG)]

PSOutput ps(VSOutput a_input)
{
	PSOutput _out;

	float2 _uv = a_input.uv;

	// アルベド
	float4 _baseTex = g_mainTex.Sample(smp, _uv);
	if(_baseTex.w < 0.5f)
	{
		discard;
	}
	_out.albedo = _baseTex * baseColor;

	// 法線
	float3 _nTex = g_normalTex.Sample(smp, _uv).xyz * 2 - 1;
	
	float3x3 TBN =
	{
		normalize(a_input.wT),
		normalize(a_input.wB),
		normalize(a_input.wN)
	};

	float3 _wNormal = mul(_nTex, TBN);
	_out.normal = EncodeNormalOct(normalize(_wNormal));

	// マテリアル
	float3 _mr = g_metRogTex.Sample(smp, _uv).rgb;
	_out.material = float4(
		_mr.r, // AO
		_mr.g * metallicRoughness.y, // 滑らかさ
		_mr.b * metallicRoughness.x, // 金属
		1.0f // 予備
	);

	// エミッシブ
	float4 _eTex = g_emiTex.Sample(smp, _uv);
	_out.emissiv = _eTex * emissiveColor;
	_out.emissiv = _eTex;
	
	return _out;
}
