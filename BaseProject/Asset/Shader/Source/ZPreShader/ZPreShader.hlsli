#include "../RootSignatureLayout.hlsli"

#define ZPRE_ROOT_SIG \
RS_FLAGS ","\
RS_CAMERA_CB ","\
"CBV(b1,visibility = SHADER_VISIBILITY_ALL),"\
"CBV(b2,visibility = SHADER_VISIBILITY_ALL),"\
"CBV(b3,visibility = SHADER_VISIBILITY_ALL),"\
"CBV(b4,visibility = SHADER_VISIBILITY_ALL),"\
"DescriptorTable(SRV(t0, numDescriptors=4),visibility = SHADER_VISIBILITY_PIXEL),"\
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
	int4 offsetData;
};

// サンプラー
SamplerState smp : register(s0);

// テクスチャ
Texture2D g_mainTex : register(t0);
Texture2D g_emiTex : register(t1);
Texture2D g_metRogTex : register(t2);
Texture2D g_normalTex : register(t3);

struct BonePallet
{
	row_major float4x4 mat;
};
StructuredBuffer<BonePallet> g_bonePalletData : register(t4);

// 頂点シェーダー出力構造体
struct VSOutput
{
	float4 svpos : SV_Position; // 変換された座標
};


