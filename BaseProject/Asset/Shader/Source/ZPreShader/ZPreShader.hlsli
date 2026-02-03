// カメラの定数バッファ
cbuffer camera : register(b0)
{
	float4x4 cView; // ビュー行列
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

// サンプラー
SamplerState smp : register(s0);

// テクスチャ
Texture2D g_mainTex : register(t0);
Texture2D g_emiTex : register(t1);
Texture2D g_metRogTex : register(t2);
Texture2D g_normalTex : register(t3);

// 頂点シェーダー入出力構造体
struct VSInput
{
	float3 pos : POSITION; // 頂点座標
};

// 頂点シェーダー出力構造体
struct VSOutput
{
	float4 svpos : SV_Position; // 変換された座標
};


