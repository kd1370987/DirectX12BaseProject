// インクルードガード
#ifndef SRV_BONEPALLET_DATA_HLSLI
#define SRV_BONEPALLET_DATA_HLSLI

struct BonePallet
{
	row_major float4x4 mat;
};
StructuredBuffer<BonePallet> g_bonePalletData : register(t2);

#endif

// 共通定数バッファ
#define RS_BONEPALLET_DATA_TABLE "DescriptorTable(SRV(t2, numDescriptors=1),visibility = SHADER_VISIBILITY_ALL)"
