#include "../../RootSignatureIncl/ForwardLightingRoot.hlsli"

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

	float3 _specular = _albedo.rgb;									// スペキュラー

	_albedo *= baseColor;

	// 出力カラー
	//float4 _outColor = {1.0f,0.0f,1.0f,1.0f };

	float3 _outColor = { 0.0f, 0.0f, 0.0f };

	// ピクセル座標へのベクトル
	float3 _V = normalize(a_input.svpos).xyz;

	// 平行光
	float3 _L = normalize(-DL_dir);
	float _NdotL = saturate(dot(_normal,_L));

	// ディズニーベースの物理ベースライティング
	// 拡散反射
	float _diffuseFromFresnel = CalcDiffuseFromFresnel(
		_normal,
		_L,
		_V,
		0.5f
	);

	// 正規化Lambert拡散反射
	float3 _lambertDiffuse = DL_color * _NdotL / PI;

	// 最終的な拡散反射光を計算
	float3 _diffuse = _albedo.rgb * _diffuseFromFresnel * _lambertDiffuse;

	// Cook-Toranceモデルを利用した鏡面反射を計算
	float _spec = CookTorranceSpecular(
		_L,
		_V,
		_normal,
		_metallic
	);
	_spec *= DL_color;

	// 金属度が高ければ、鏡面反射はスペキュラーカラー、低ければ白
	_spec *= lerp(float3(1.0f,1.0f,1.0f),_specular,_metallic);

	// 滑らかさを考慮した拡散反射を計算
	_outColor += _diffuse * (1.0f) + _spec;

	// アンビエント
	_outColor += ambientLightColor.rgb * _albedo.rgb;

	return float4(_outColor,_albedo.a);
}
