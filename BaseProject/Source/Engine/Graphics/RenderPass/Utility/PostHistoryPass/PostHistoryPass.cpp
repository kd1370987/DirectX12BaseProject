#include "PostHistoryPass.h"

#include "Engine/Graphics/RenderPassRegistry/RenderPassRegistry.h"
#include "Engine/Graphics/RenderContext/RenderContext.h"
#include "Engine/Graphics/RenderGraph/RenderGraph.h"
#include "Engine/Graphics/RenderGraph/RGPassBuilder/RGPassBuilder.h"
#include "Engine/Graphics/GraphicEngine.h"

namespace Engine::Graphics
{
	void AddPostHistoryPass(D3D12::PipelineStateManager* a_pPSOManager, RenderPassRegistry* a_pRegistry, const EDrawPhase& a_phase)
	{
		// TAAテクスチャをコピーするだけのパスなので、宣言だけで完結する
		RenderPassNode _node = {};
		_node.name = "PostHistoryPass";
		_node.phase = a_phase;
		RGGlobalsPassBuilder _rpBuilder(&_node);

		_rpBuilder.Copy("AfterTAAColor", "HistoryTAAColor", DXGI_FORMAT_R8G8B8A8_UNORM);

		a_pRegistry->RegisterPass(_node);
	}
}
