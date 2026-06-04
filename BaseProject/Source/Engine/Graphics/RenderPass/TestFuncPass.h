#pragma once

#include "../RenderGraph/RGData/RenderPassNode.h"

namespace Engine::D3D12
{
	class PipelineStateManager;
}

namespace Engine::Graphics
{
	class RenderGraph;

	void AddTestFuncPass(D3D12::PipelineStateManager* a_pPSOManager,RenderGraph& a_rg,const EDrawPhase& a_phase);
}