// ルートパラメーターの構造体
#include "../../../Common/RootParameters/Vertex.hlsli"
#include "../../../Common/RootParameters/BonePalletData.hlsli"
#include "../../../Common/RootParameters/SkinningInfo.hlsli"

//==========================================================================================
// ルートパラメーター
//
//   0 : CBV(b0)         このディスパッチが担当する範囲
//   1 : SRVテーブル(t0) ボーン行列
//   2 : SRVテーブル(t1) 頂点メガバッファ
//   3 : SRVテーブル(t2) インデックスメガバッファ
//   4 : UAVテーブル(u0) 変形後頂点の書き込み先
//==========================================================================================
#define SKINNING_ROOT_SIG \
"RootFlags(0)," \
"CBV(b0)," \
"DescriptorTable(SRV(t0, numDescriptors=1)), " \
"DescriptorTable(SRV(t1, numDescriptors=1)), " \
"DescriptorTable(SRV(t2, numDescriptors=1)), " \
"DescriptorTable(UAV(u0, numDescriptors=1))"

cbuffer CBSkinningInfo : register(b0)
{
	SkinningInfo g_info;
}

StructuredBuffer<BonePallet>	g_bonePalletData	: register(t0);
StructuredBuffer<Vertex>		g_vertexfloatData	: register(t1);
StructuredBuffer<uint>			g_indexData			: register(t2);

RWStructuredBuffer<Vertex>		g_outputVertex		: register(u0);
