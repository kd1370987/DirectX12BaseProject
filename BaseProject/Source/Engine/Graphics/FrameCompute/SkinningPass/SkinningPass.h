#pragma once

namespace Engine::Graphics
{
	class GraphicsEngine;
	class RenderContext;
	class PipelineStateManager;

	//======================================================================================
	// スキニング
	//
	// カメラに依存せず、フレームに1回で足りる計算なのでレンダーグラフには載せない。
	// GraphicsEngine が直接呼ぶ
	//======================================================================================
	// ルートシグネチャとPSOの用意(初期化時に1回)
	void SetupSkinning(PipelineStateManager* a_pPSOManager);

	// 実行(毎フレーム1回)
	void ExecuteSkinning(GraphicsEngine* a_pGE, RenderContext* a_pCtx);
}
