#include "GBufferHistoryPass.h"

#include "Engine/Graphics/RenderPassRegistry/RenderPassRegistry.h"
#include "Engine/Graphics/RenderContext/RenderContext.h"
#include "Engine/Graphics/RenderGraph/RenderGraph.h"
#include "Engine/Graphics/RenderGraph/RGPassBuilder/RGPassBuilder.h"
#include "Engine/Graphics/GraphicEngine.h"

namespace Engine::Graphics
{
	void AddGBufferHistoryPass(D3D12::PipelineStateManager* a_pPSOManager, RenderPassRegistry* a_pRegistry, const EDrawPhase& a_phase)
	{
		// コピーするだけのパスなので、宣言だけで完結する（実行関数は不要）
		RenderPassNode _node = {};
		_node.name = "GBufferHistoryPass";
		_node.phase = a_phase;
		RGGlobalsPassBuilder _rpBuilder(&_node);

		_rpBuilder.Copy("Depth", "PrevDepth", DXGI_FORMAT_R32_FLOAT);
		_rpBuilder.Copy("GBufferNormal", "PrevNormal", DXGI_FORMAT_R16G16_FLOAT);

		a_pRegistry->RegisterPass(_node);
	}
}
