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
		RenderPassNode _node = {};
		_node.name = "PostHistoryPass";
		RGGlobalsPassBuilder _rpBuilder(&_node);

		_rpBuilder.CopySrc("AffterTAAColor");

		_rpBuilder.CopyDst("HistoryTAAColor", DXGI_FORMAT_R8G8B8A8_UNORM);

		struct RuntimeData
		{
			
		};
		auto _spPassData = std::make_shared<RuntimeData>();
		

		_node.executeFunc = [_spPassData](GraphicsEngine* a_pGE, RenderContext* a_pCtx, uint8_t a_passIndex)
			{
				// TAAテクスチャ
				const auto& _srcTAATexHandle = a_pGE->RefRenderGraph()->GetTexHandle("AffterTAAColor");
				const auto& _dstTAATexHandle = a_pGE->RefRenderGraph()->GetTexHandle("HistoryTAAColor");
				a_pCtx->TexCopy(_srcTAATexHandle, _dstTAATexHandle);
			};

		_node.phase = a_phase;
		a_pRegistry->RegisterPass(_node);
	}
}