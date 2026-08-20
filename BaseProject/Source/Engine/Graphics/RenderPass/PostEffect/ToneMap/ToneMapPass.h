#pragma once

#include "Engine/Graphics/RenderGraph/RGData/RenderPassNode.h"

namespace Engine::D3D12
{
	class PipelineStateManager;
}

namespace Engine::Graphics
{
	class RenderPassRegistry;

	// HDR の最終カラーをトーンマップして FinalColor(最終テクスチャ)へ書き出すパス
	void AddToneMapPass(D3D12::PipelineStateManager* a_pPSOManager, RenderPassRegistry* a_pRegistry, const EDrawPhase& a_phase);
}
