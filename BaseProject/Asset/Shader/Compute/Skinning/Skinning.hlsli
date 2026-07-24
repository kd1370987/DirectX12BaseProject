#include "../../Common/Struct/Vertex.hlsli"

struct BonePallet
{
	row_major float4x4 mat;
};

struct SkinningInfo
{
	uint vertexStart;			// 頂点のスタートインデックス
	uint animatedVertStart;
	uint vertexCount;			// キャラの頂点数
	uint boneOffset;			// このキャラのボーンの開始場所
};

cbuffer CBSkinningInfo : register(b0)
{
	SkinningInfo g_info;
}

StructuredBuffer<BonePallet>	g_bonePalletData	: register(t0);		// ボーン行列
StructuredBuffer<Vertex>		g_vertexfloatData	: register(t1);		// メッシュメガバッファ
StructuredBuffer<uint>			g_indexData			: register(t2);		// インデックスメガバッファ

RWStructuredBuffer<Vertex>		g_outputVertex		: register(u0);		// 変形後頂点出力先



// ルートシグネチャデータ
#define SKINNING_ROOT_SIG \
"RootFlags(0)," \
"CBV(b0),"\
"DescriptorTable(SRV(t0, numDescriptors=1)), " \
"DescriptorTable(SRV(t1, numDescriptors=1)), " \
"DescriptorTable(SRV(t2, numDescriptors=1)), " \
"DescriptorTable(UAV(u0, numDescriptors=1))"
