#include "../RootSignatureLayout.hlsli"

#define SCREEN2D_ROOT_SIG \
"RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT), " \
"CBV(b0), " \
"DescriptorTable(SRV(t0, numDescriptors=1)), " \
RS_STATIC_SAMPLER


// 定数バッファ
cbuffer cbUI : register(b0)
{
	float4x4 mat; // ワールド行列
	float4 color; // 色
};

// テクスチャ
Texture2D g_mainTex : register(t0);

// サンプラー
SamplerState g_smp : register(s0);

// 頂点シェーダー入力
struct VSInput
{
	float4 pos : POSITION; // 頂点座標
	float2 uv : TEXCOORD; // uv座標
};

// 頂点シェーダー出力
struct VSOutput
{
	float4 svPos : SV_POSITION; // 変換された座標
	float2 uv : TEXCOORD; // uv座標
};
