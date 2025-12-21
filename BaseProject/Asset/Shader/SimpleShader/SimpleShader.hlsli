// カメラの定数バッファ
cbuffer camera : register(b0)
{
	float4x4 cView; // ビュー行列
	float4x4 cProj; // 投影行列
	float4x4 cProjInv; // 投影行列の逆行列

	float3 cCameraPos; // カメラ位置
	float padding1; // パディング
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
Texture2D _MainTex : register(t0);

// 頂点シェーダー入出力構造体
struct VSInput
{
	float3 pos : POSITION; // 頂点座標
	float3 normal : NORMAL; // 法線
	float2 uv : TEXCOORD; // uv座標
	float3 tangent : TANGENT; // 接空間
	float4 color : COLOR; // 頂点色
};

// 頂点シェーダー出力構造体
struct VSOutput
{
	float4 svpos : SV_Position; // 変換された座標
	float4 color : COLOR; // 変換された色
	float2 uv : TEXCOORD; // uv座標
};


