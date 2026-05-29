// インクルードガード
#ifndef SRV_SUBSET_DATA_HLSLI
#define SRV_SUBSET_DATA_HLSLI

struct SubsetData
{
	float4 baseColorScale;
	float3 emissiveColorScale;

	int metallic;
	int roughness;

	float3 pad;
};

// インスタンスデータ
StructuredBuffer<SubsetData> g_subsetData : register(t1);

#endif

// 共通バッファ
#define RS_SUBSET_DATA_TABLE "DescriptorTable(SRV(t1, numDescriptors=1),visibility = SHADER_VISIBILITY_PIXEL)"
