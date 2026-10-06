#pragma once

#include "Engine/Common/Handle.h"

namespace Engine::Resource
{
	class ResourceManager;
}

namespace Engine::Graphics
{
	class GraphicsEngine;
	class RenderContext;
	class PipelineStateManager;

	//======================================================================================
	// GPUパーティクルのシミュレーション
	//
	// 発生(Emit)と更新(Update)は、カメラに依存せずフレームに1回で足りる計算なので
	// レンダーグラフのパスにはしていない。GraphicsEngine が持ち、直接呼ぶ。
	//
	// 2つを1つの関数にまとめてあるのは、間に挟むUAVバリアを外せなくするため。
	// 両者は同じ deadList / counter を触るので、バリアが無いと
	// Dispatch がGPU上で並列に走り、空きスロットが少しずつ減ってエミットが先細りする。
	// パスに分かれていると「間に別のパスが入る」余地が残るが、
	// ここへまとめておけば順序とバリアが崩れようがない
	//======================================================================================
	class ParticleSimulation
	{
	public:

		// ルートシグネチャとPSOの用意(初期化時に1回)
		// a_resourceManager : シェーダーを読み込む先。更新でもアセットの値を引くので持っておく
		void Setup(PipelineStateManager* a_pPSOManager, Resource::ResourceManager& a_resourceManager);

		// 実行(毎フレーム1回) : 発生 -> UAVバリア -> 更新
		void Execute(GraphicsEngine* a_pGE, RenderContext* a_pCtx);

	private:

		PipelineStateManager* m_pPSOManager = nullptr;
		Resource::ResourceManager* m_pResourceManager = nullptr;	// アセットの値を引く(借り物)

		// 各シェーダーに対応する組み合わせ
		Handle<ID3D12RootSignature> m_emitRootSig = {};
		Handle<ID3D12PipelineState> m_emitPSO = {};

		Handle<ID3D12RootSignature> m_updateRootSig = {};
		Handle<ID3D12PipelineState> m_updatePSO = {};

		Handle<ID3D12RootSignature> m_resetRootSig = {};
		Handle<ID3D12PipelineState> m_resetPSO = {};

		// プールを伸ばしたあと、増えた範囲を使えるようにする
		Handle<ID3D12RootSignature> m_growRootSig = {};
		Handle<ID3D12PipelineState> m_growPSO = {};

		// このフレームの乱数の種
		uint32_t m_frameCounter = 0;
	};
}
