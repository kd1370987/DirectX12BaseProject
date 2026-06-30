#include "PostHistoryPass.h"

#include "Engine/Graphics/RenderContext/RenderContext.h"
#include "Engine/Graphics/RenderGraph/RenderGraph.h"
#include "Engine/Graphics/RenderGraph/RGPassBuilder/RGPassBuilder.h"

namespace Engine::Graphics
{
	void AddPostHistoryPass(D3D12::PipelineStateManager* a_pPSOManager, RenderGraph& a_rg, const EDrawPhase& a_phase)
	{
		RenderPassNode _node = {};
		_node.name = "PostHistoryPass";
		RGGlobalsPassBuilder _rpBuilder(&_node, &a_rg);

		_rpBuilder.CopySrc("AffterTAAColor");

		_rpBuilder.CopyDst("HistoryTAAColor", DXGI_FORMAT_R8G8B8A8_UNORM);

		struct RuntimeData
		{
			RenderGraph* pRG;
		};
		auto _spPassData = std::make_shared<RuntimeData>();
		_spPassData->pRG = &a_rg;

		_node.executeFunc = [_spPassData](GraphicsEngine* a_pGE, RenderContext* a_pCtx, uint8_t a_passIndex)
			{
				// TAAテクスチャ
				const auto& _srcTAATexHandle = _spPassData->pRG->GetTexHandle("AffterTAAColor");
				const auto& _dstTAATexHandle = _spPassData->pRG->GetTexHandle("HistoryTAAColor");
				a_pCtx->TexCopy(_srcTAATexHandle, _dstTAATexHandle);
			};

		a_rg.AddPassNode(a_phase, _node);
	}
}