//==========================================================================================
// 間接描画の引数をフレームの頭で書き直す(1スレッドだけ)
//
// 引数は D3D12_DRAW_INDEXED_ARGUMENTS(uint ×5)。ParticlePass が ExecuteIndirect で読む。
//   1段目 : インスタンス数 = 容量(今までの DrawIndexedInstanced と同じ)
//   2段目 : インスタンス数 = 0 から始め、Update が生き残った粒を数えて足す
//
// ルートパラメーター
//   0 : ルート定数(b100) 引数バッファの UAV 番号 / インデックス数 / インスタンス数
//==========================================================================================
#define RESETDRAWARGS_ROOT_SIG \
	"RootFlags(CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED),"\
	"RootConstants(num32BitConstants=3, b100)"

cbuffer ResetParam : register(b100)
{
	uint g_drawArgsIndex;		// 間接引数バッファの UAV 番号
	uint g_indexCount;			// 板ポリのインデックス数(QuadPolygon::GetIndexCount)
	uint g_instanceCount;		// 1段目 : 容量 / 2段目 : 0(Update が数える)
}

[RootSignature(RESETDRAWARGS_ROOT_SIG)]
[numthreads(1, 1, 1)]
void CSMain()
{
	// 描画命令用の引数を作成
	RWStructuredBuffer<uint> _args = ResourceDescriptorHeap[g_drawArgsIndex];
	_args[0] = g_indexCount;			// IndexCountPerInstance
	_args[1] = g_instanceCount;			// InstanceCount
	_args[2] = 0;						// StartIndexLocation
	_args[3] = 0;						// BaseVertexLocation
	_args[4] = 0;						// StartInstanceLocation
}
