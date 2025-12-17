// カメラの定数バッファ
cbuffer camera : register(b0)
{
	float4x4 cView; // ビュー行列
	float4x4 cProj; // 投影行列
	float4x4 cProjInv; // 投影行列の逆行列

	float3 cCameraPos; // カメラ位置
	float padding1; // パディング
}

// ワールド変換の定数バッファ
cbuffer Transform : register(b1)
{
	float4x4 mat; // ワールド行列
}

//cbuffer CBObject : register(b1)
//{
//	float2 uvOffset; // UVオフセット
//	float2 uvTiling; // UVタイル
//}

//cbuffer Transform : register(b2)
//{
//	float4x4 mat; // ワールド行列
//}


//cbuffer CBMaterial : register(b3)
//{
//	float4 baseColor; // ベースカラー
//	float3 emissiveColor; // エミッシブカラー
//	float metallic; // メタリック
//	float roughness; // ラフネス
//}

SamplerState smp : register(s0); // サンプラー


Texture2D _MainTex : register(t0); // テクスチャ

struct VSInput
{
	float3 pos : POSITION; // 頂点座標
	float3 normal : NORMAL; // 法線
	float2 uv : TEXCOORD; // uv座標
	float3 tangent : TANGENT; // 接空間
	float4 color : COLOR; // 頂点色
};

struct VSOutput
{
	float4 svpos : SV_Position; // 変換された座標
	float4 color : COLOR; // 変換された色
	float2 uv : TEXCOORD; // uv座標
};


