// ルートパラメーターの構造体
#include "../../../Common/RootParameters/Vertex.hlsli"
#include "../../../Common/RootParameters/BonePalletData.hlsli"
#include "../../../Common/RootParameters/SkinningInfo.hlsli"

//==========================================================================================
// ルートパラメーター
//
//   0 : CBV(b0)         このディスパッチが担当する範囲
//   1 : SRVの番号(t0) ボーン行列
//   2 : SRVの番号(t1) 頂点メガバッファ
//   3 : SRVの番号(t2) インデックスメガバッファ
//   4 : UAVの番号(u0) 変形後頂点の書き込み先
//==========================================================================================
#define SKINNING_ROOT_SIG \
"RootFlags(CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED)," \
"CBV(b0)," \
"RootConstants(num32BitConstants=1, b100), " \
"RootConstants(num32BitConstants=1, b101), " \
"RootConstants(num32BitConstants=1, b102), " \
"RootConstants(num32BitConstants=1, b103)"

cbuffer CBSkinningInfo : register(b0)
{
	SkinningInfo g_info;
}

// SRVの番号(ResourceDescriptorHeap の添字)。ルート定数で届く
cbuffer PassDescriptorIndex0 : register(b100)
{
	uint g_bonePalletDataIndex;
}

StructuredBuffer<BonePallet> Get_bonePalletData() { StructuredBuffer<BonePallet> _r = ResourceDescriptorHeap[g_bonePalletDataIndex]; return _r; }
#define g_bonePalletData Get_bonePalletData()
// SRVの番号(ResourceDescriptorHeap の添字)。ルート定数で届く
cbuffer PassDescriptorIndex1 : register(b101)
{
	uint g_vertexfloatDataIndex;
}

StructuredBuffer<Vertex> Get_vertexfloatData() { StructuredBuffer<Vertex> _r = ResourceDescriptorHeap[g_vertexfloatDataIndex]; return _r; }
#define g_vertexfloatData Get_vertexfloatData()
// SRVの番号(ResourceDescriptorHeap の添字)。ルート定数で届く
cbuffer PassDescriptorIndex2 : register(b102)
{
	uint g_indexDataIndex;
}

StructuredBuffer<uint> Get_indexData() { StructuredBuffer<uint> _r = ResourceDescriptorHeap[g_indexDataIndex]; return _r; }
#define g_indexData Get_indexData()

// UAVの番号(ResourceDescriptorHeap の添字)。ルート定数で届く
cbuffer PassDescriptorIndex3 : register(b103)
{
	uint g_outputVertexIndex;
}

RWStructuredBuffer<Vertex> Get_outputVertex() { RWStructuredBuffer<Vertex> _r = ResourceDescriptorHeap[g_outputVertexIndex]; return _r; }
#define g_outputVertex Get_outputVertex()
