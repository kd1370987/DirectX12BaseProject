
#include "../RootSignatureLayout.hlsli"

#define GBUFFER_ROOT_SIG \
"RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT), " \
RS_CAMERA_CB ","\
"CBV(b1), CBV(b2), CBV(b3), CBV(b4), " \
"DescriptorTable(SRV(t0, numDescriptors=4)), " \
RS_STATIC_SAMPLER

// カメラの定数バッファ
cbuffer camera : register(b0)
{
	float4x4 cView; // ビュー行列
	float4x4 cViewInv; // ビュー行列
	float4x4 cProj; // 投影行列
	float4x4 cProjInv; // 投影行列の逆行列

	float4 cCameraPos; // カメラ位置
}

// オブジェクトの定数バッファ
cbuffer CBObject : register(b1)
{
	float4 uvTransform; // UV変換 (x: offsetU, y: offsetV, z: tilingU, w: tilingV)
}

cbuffer Transform : register(b2)
{
	float4x4 mat; // ワールド行列
}

cbuffer CBMaterial : register(b3)
{
	float4 baseColor; // ベースカラー
	float4 emissiveColor; // エミッシブカラー
	float4 metallicRoughness; // めたりっくラフネス	(x: metallic, y: roughness, z: unused, w: unused)
}
cbuffer cbBones : register(b4)
{
	row_major float4x4 g_mBones[300];
};

// サンプラー
SamplerState smp : register(s0);

// テクスチャ
Texture2D g_mainTex : register(t0);
Texture2D g_metRogTex : register(t1);
Texture2D g_emiTex : register(t2);
Texture2D g_normalTex : register(t3);


// 頂点シェーダー出力構造体
struct VSOutput
{
	float4 svpos : SV_Position; // 変換された座標
	float4 color : COLOR; // 変換された色
	float2 uv : TEXCOORD; // uv座標
	float3 normal : NORMAL; // 法線
	
	float3 wN : TEXCOORD1; // ワールド法線
	float3 wT : TEXCOORD2; // ワールド接線
	float3 wB : TEXCOORD3; // ワールド副接線(従法線)
};

// GBuffer用出力
struct PSOutput
{
	float4 albedo : SV_Target0;
	float2 normal : SV_Target1;
	float4 material : SV_Target2;
	float4 emissiv : SV_Target3;
};




