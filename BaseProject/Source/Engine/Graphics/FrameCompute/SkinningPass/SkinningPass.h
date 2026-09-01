#pragma once

namespace Engine::D3D12
{
	class PipelineStateManager;
}

namespace Engine::Graphics
{

	//======================================================================================
	// スキニング
	//
	// カメラに依存せず、フレームに1回で足りる計算なのでレンダーグラフには載せない。
	// GraphicsEngine が直接呼ぶ
	//======================================================================================
	// ルートシグネチャとPSOの用意(初期化時に1回)
	void SetupSkinning(D3D12::PipelineStateManager* a_pPSOManager);

	// 実行(毎フレーム1回)
	void ExecuteSkinning(GraphicsEngine* a_pGE, RenderContext* a_pCtx);
}
