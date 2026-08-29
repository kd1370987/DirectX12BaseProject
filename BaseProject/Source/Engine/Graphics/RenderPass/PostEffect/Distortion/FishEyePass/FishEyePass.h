#pragma once

#include "Engine/Graphics/RenderGraph/RGData/RenderPassNode.h"

namespace Engine::D3D12
{
	class PipelineStateManager;
}

namespace Engine::Graphics
{
	class RenderGraph;
	class RenderPassRegistry;

	// メインカラーを中心から外へ引き伸ばして、魚眼レンズ越しの歪みを出すパス
	void AddFishEyePass(D3D12::PipelineStateManager* a_pPSOManager, RenderPassRegistry* a_pRegistry, const EDrawPhase& a_phase);
}
