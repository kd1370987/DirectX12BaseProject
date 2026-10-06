#include "../../../Common/RootParameters/Particle.hlsli"

//==========================================================================================
// プールの容量を伸ばしたあと、増えた範囲を使えるようにする
//
// GPUParticlePool::BeginGrow が新しいバッファを作って、古い粒とデッドリストを先頭へ写してある。
// ここでは増えた範囲 [oldCapacity, oldCapacity + addCount) を
//   ・粒は 0 で埋める(life = 0 なので更新は素通りし、描かれもしない)
//   ・番号をデッドリスト(空き番号のスタック)の上へ積む
// そのあとカウンター(スタックの高さ)を増えたぶんだけ足す。
//
// 積む位置はカウンターの今の値から決まり、カウンターは GPU にしか無いので CS で行う。
// 同じ Dispatch の中で全スレッドがカウンターを読みながら1人が足すと読み書きが食い違うので、
// 「埋めて積む」(mode 0)と「カウンターを足す」(mode 1、1スレッド)を分け、間に UAV バリアを挟む
//
// ルートパラメーター
//   0 : ルート定数(b100) 粒 / デッドリスト / カウンターの UAV 番号、前の容量、増えた数、モード
//==========================================================================================
#define GROWPARTICLEPOOL_ROOT_SIG \
	"RootFlags(CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED),"\
	"RootConstants(num32BitConstants=6, b100)"

cbuffer GrowParam : register(b100)
{
	uint g_particleBufferIndex;		// 粒(新しいバッファ)
	uint g_deadListIndex;			// デッドリスト(新しいバッファ)
	uint g_counterBufferIndex;		// カウンター(作り直していない)
	uint g_oldCapacity;				// 伸ばす前の容量 = 増えた範囲の先頭の番号
	uint g_addCount;				// 増えた数
	uint g_mode;					// 0 : 埋めて積む / 1 : カウンターを足す
}

// ※ C++ 側の Dispatch(ParticleSimulation.cpp)のスレッド数 64 と合わせること
[RootSignature(GROWPARTICLEPOOL_ROOT_SIG)]
[numthreads(64, 1, 1)]
void CSMain(uint3 DTid : SV_DispatchThreadID)
{
	RWStructuredBuffer<uint> _counter = ResourceDescriptorHeap[g_counterBufferIndex];

	if (g_mode == 1)
	{
		// カウンターを足す(1スレッドだけ)
		if (DTid.x == 0)
		{
			_counter[0] = _counter[0] + g_addCount;
		}
		return;
	}

	const uint _k = DTid.x;
	if (_k >= g_addCount) return;

	RWStructuredBuffer<ParticleData> _particles = ResourceDescriptorHeap[g_particleBufferIndex];
	RWStructuredBuffer<uint> _deadList = ResourceDescriptorHeap[g_deadListIndex];

	const uint _index = g_oldCapacity + _k;

	// 全メンバーを 0 にする(未初期化のまま UAV へ書くと DXC の検証で落ちる)
	_particles[_index] = (ParticleData)0;

	// 空き番号のスタックの上へ積む。高さ(カウンター)はこの Dispatch の間は誰も変えない
	_deadList[_counter[0] + _k] = _index;
}
