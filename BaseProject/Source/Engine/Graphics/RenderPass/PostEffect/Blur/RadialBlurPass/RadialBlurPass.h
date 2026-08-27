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

	// メインカラーを中心から放射状に引きずってボカすパス
	void AddRadialBlurPass(D3D12::PipelineStateManager* a_pPSOManager, RenderPassRegistry* a_pRegistry, const EDrawPhase& a_phase);
}
