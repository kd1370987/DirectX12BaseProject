#include "GBufferShader.hlsli"

PSOutput ps(VSOutput a_input)
{
	PSOutput _out;

	float2 _uv = a_input.uv;

	// アルベド
	float4 _baseTex = g_mainTex.Sample(smp,_uv);
	_out.albedo = _baseTex * baseColor;

	// 法線
	float3 _nTex = g_normalTex.Sample(smp, _uv).xyz * 2 - 1;

	float3x3 TBN =
		{
		normalize(a_input.wT),
		normalize(a_input.wB),
		normalize(a_input.wN)
	};

	float3 _wNormal = mul(_nTex,TBN);
	_out.normal = EncodeNormalOct(normalize(_wNormal));

	// マテリアル
	float2 _mr = g_metRogTex.Sample(smp, _uv).rg;
	_out.material = float4(
		_mr.r,
		_mr.g,
		1.0,					// AO仮
		emissiveColor.r
	);
	
	return _out;

	
}
