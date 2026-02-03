// カメラの定数バッファ
cbuffer camera : register(b0)
{
	float4x4 cView; // ビュー行列
	float4x4 cProj; // 投影行列
	float4x4 cProjInv; // 投影行列の逆行列

	float4 cCameraPos; // カメラ位置
}

// ディファードレンダリングでは共通
Texture2D g_albedoTex : register(t0);
Texture2D g_normalTex : register(t1);
Texture2D g_materialTex : register(t2);
Texture2D g_depthTex : register(t3);

// サンプラー
SamplerState g_samp : register(s0);

// 頂点出力
struct VSOutput
{
	float4 pos	: SV_POSITION;
	float2 uv	: TEXCOORD;
};
