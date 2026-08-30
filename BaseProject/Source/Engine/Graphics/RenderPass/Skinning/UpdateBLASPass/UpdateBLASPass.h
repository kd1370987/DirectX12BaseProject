#pragma once

#include "Engine/Graphics/RenderGraph/RGData/RenderPassNode.h"

namespace Engine::D3D12
{
	class PipelineStateManager;
}

namespace Engine::Graphics
{
	class RenderPassRegistry;

	//======================================================================================
	// スキニング結果からのBLAS更新
	//
	// カメラに依存せず、フレームに1回で足りる計算なのでレンダーグラフには載せない
	//======================================================================================
	void ExecuteUpdateBLAS(GraphicsEngine* a_pGE, RenderContext* a_pCtx);
}
