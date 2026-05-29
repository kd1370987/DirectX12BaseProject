// インクルードガード
#ifndef SRV_INSTANCE_DATA_HLSLI
#define SRV_INSTANCE_DATA_HLSLI

struct InstanceData
{
	float4x4 worldMat;
	float4x4 prevWorldMat;

	int boneStartIndex;
	int boneCount;

	float2 pad;
};

// インスタンスデータ
StructuredBuffer<InstanceData> g_instanceData : register(t0);

#endif

// 共通バッファ
#define RS_INSTANCE_DATA_TABLE "DescriptorTable(SRV(t0, numDescriptors=1),visibility = SHADER_VISIBILITY_ALL)"
