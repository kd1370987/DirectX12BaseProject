#pragma once

namespace Engine::D3D12
{
	class PipelineStateManager;
}

namespace Engine::Graphics
{
	class GraphicsEngine;
	class RenderContext;

	//======================================================================================
	// GPUパーティクルのシミュレーション
	//
	// 発生(Emit)と更新(Update)は、カメラに依存せずフレームに1回で足りる計算なので
	// レンダーグラフのパスにはしていない。GraphicsEngine が直接呼ぶ。
	//
	// 2つを1つの関数にまとめてあるのは、間に挟むUAVバリアを外せなくするため。
	// 両者は同じ deadList / counter を触るので、バリアが無いと
	// Dispatch がGPU上で並列に走り、空きスロットが少しずつ減ってエミットが先細りする。
	// パスに分かれていると「間に別のパスが入る」余地が残るが、
	// ここへまとめておけば順序とバリアが崩れようがない
	//======================================================================================
	// ルートシグネチャとPSOの用意(初期化時に1回)
	void SetupParticleSimulation(D3D12::PipelineStateManager* a_pPSOManager);

	// 実行(毎フレーム1回) : 発生 -> UAVバリア -> 更新
	void ExecuteParticleSimulation(GraphicsEngine* a_pGE, RenderContext* a_pCtx);
}
