// 周りから来る光(GI・反射・環境光)のうち、鏡面へ回る割合。
//
// 本来は「F0・粗さ・視線の角度」から引く2次元のLUT(Split Sum の環境BRDF)を使うが、
// テクスチャを持たずに済むよう、その近似式で求める(Karis, "Physically Based Shading on Mobile")。
//
//   ・滑らかな面ほど、また視線が浅いほど 1 へ近づく(フレネル)
//   ・粗い面ほど浅い角度での立ち上がりが鈍る(Schlick をそのまま使うと粗い面の縁が光りすぎる)
//
// 拡散へ回る割合は、残り (1 - これ) に (1 - metallic) を掛けたもの
#ifndef ENV_BRDF_HLSLI
#define ENV_BRDF_HLSLI

// @param a_F0        垂直入射のときの反射率(非金属は 0.04 前後 / 金属はアルベド)
// @param a_roughness 粗さ(0 = 鏡 / 1 = ざらざら)
// @param a_NdotV     法線と視線(面から目へ)の内積
float3 EnvBRDFApprox(float3 a_F0, float a_roughness, float a_NdotV)
{
	const float4 _c0 = float4(-1.0f, -0.0275f, -0.572f, 0.022f);
	const float4 _c1 = float4(1.0f, 0.0425f, 1.04f, -0.04f);

	float4 _r = a_roughness * _c0 + _c1;
	float _a004 = min(_r.x * _r.x, exp2(-9.28f * a_NdotV)) * _r.x + _r.y;
	float2 _AB = float2(-1.04f, 1.04f) * _a004 + _r.zw;

	return a_F0 * _AB.x + _AB.y;
}

#endif
