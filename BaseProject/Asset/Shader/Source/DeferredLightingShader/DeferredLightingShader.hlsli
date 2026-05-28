#include "../RootSignatureLayout.hlsli"

#define DEFERRED_ROOT_SIG \
RS_FLAGS","\
RS_CAMERA_CB ","\
"CBV(b1,visibility = SHADER_VISIBILITY_ALL), " \
"DescriptorTable(SRV(t0, numDescriptors=7),visibility = SHADER_VISIBILITY_PIXEL), " \
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

cbuffer cbAmbient : register(b1)
{
	float4 g_ambientColor;

	float4 g_DL_Dir; // ライトの方向（ワールド空間）
	float4 g_DL_Color; // ライトの色
};

// ディファードレンダリングでは共通
Texture2D g_albedoTex : register(t0);
Texture2D g_normalTex : register(t1);
Texture2D g_materialTex : register(t2);
Texture2D g_emiTex : register(t3);
Texture2D g_depthTex : register(t4);
Texture2D g_shadowMask : register(t5);
Texture2D g_rayGI : register(t6);

// サンプラー
SamplerState g_samp : register(s0);

// 頂点出力
struct VSOutput
{
	float4 pos	: SV_POSITION;
	float2 uv	: TEXCOORD;
};
