#pragma once

#include "Engine/Graphics/RenderGraph/RGData/RenderPassNode.h"

namespace Engine::D3D12
{
	class PipelineStateManager;
}

namespace Engine::Graphics
{
	class RenderPassRegistry;

	// 最終テクスチャ(FinalColor)をバックバッファへコピーするだけのパス
	void AddCopyToBackBufferPass(D3D12::PipelineStateManager* a_pPSOManager, RenderPassRegistry* a_pRegistry, const EDrawPhase& a_phase);
}
