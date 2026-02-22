// カメラの定数バッファ
cbuffer camera : register(b0)
{
	float4x4 mView; // ビュー行列
	float4x4 mProj; // 投影行列
	float4x4 mProjInv; // 投影行列の逆行列

	float4 cameraPos; // カメラ位置
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
	float4 pos : SV_Position; // 射影座標
	float3 wPos : TEXCOORD0; // ワールド3D座標

	float2 uv : TEXCOORD1; // UV座標
	float4 color : TEXCOORD2; // 色

	float3 wN : TEXCOORD3; // ワールド法線
	float3 wT : TEXCOORD4; // ワールド接線
	float3 wB : TEXCOORD5; // ワールド従法線
};

// GBuffer用出力
struct PSOutput
{
	float4 albedo : SV_Target0;
	float2 normal : SV_Target1;
	float4 material : SV_Target2;
	float4 emissiv : SV_Target3;
};




