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
	// スキニング
	//
	// カメラに依存せず、フレームに1回で足りる計算なのでレンダーグラフには載せない。
	// GraphicsEngine が持ち、直接呼ぶ
	//======================================================================================
	class SkinningCompute
	{
	public:

		// ルートシグネチャとPSOの用意(初期化時に1回)
		// a_resourceManager : シェーダーを読み込む先
		void Setup(PipelineStateManager* a_pPSOManager, Resource::ResourceManager& a_resourceManager);

		// 実行(毎フレーム1回)
		void Execute(GraphicsEngine* a_pGE, RenderContext* a_pCtx) const;

	private:

		// PSOはハンドルで持つ : 8bitの添字へ落とすと256個目から別のPSOを引く
		Handle<ID3D12RootSignature> m_rootSigHandle = {};
		Handle<ID3D12PipelineState> m_psoHandle = {};

		PipelineStateManager* m_pPSOManager = nullptr;	// 借り物
	};
}
