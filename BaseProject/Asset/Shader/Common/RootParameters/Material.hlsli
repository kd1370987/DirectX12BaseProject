// インクルードガード
#ifndef SRV_MATERIAL_HLSLI
#define SRV_MATERIAL_HLSLI

// テクスチャ
Texture2D g_mainTex : register(t3);
Texture2D g_metRogTex : register(t4);
Texture2D g_emiTex : register(t5);
Texture2D g_normalTex : register(t6);

#endif

// 共通定数バッファ
#define RS_MATERIAL_TABLE "DescriptorTable(SRV(t3, numDescriptors=4),visibility = SHADER_VISIBILITY_PIXEL)"
