#include "GBufferHistoryPass.h"

#include "Engine/Graphics/RenderContext/RenderContext.h"
#include "Engine/Graphics/RenderGraph/RenderGraph.h"
#include "Engine/Graphics/RenderGraph/RGPassBuilder/RGPassBuilder.h"

namespace Engine::Graphics
{
	void AddGBufferHistoryPass(D3D12::PipelineStateManager* a_pPSOManager, RenderGraph& a_rg, const EDrawPhase& a_phase)
	{
		RenderPassNode _node = {};
		_node.name = "GBufferHistoryPass";
		RGGlobalsPassBuilder _rpBuilder(&_node);

		_rpBuilder.CopySrc("GBufferNormal");
		_rpBuilder.CopySrc("Depth");

		_rpBuilder.CopyDst("PrevDepth", DXGI_FORMAT_R32_FLOAT);
		_rpBuilder.CopyDst("PrevNormal", DXGI_FORMAT_R16G16_FLOAT);

		struct RuntimeData
		{
			RenderGraph* pRG;
		};
		auto _spPassData = std::make_shared<RuntimeData>();
		_spPassData->pRG = &a_rg;

		_node.executeFunc = [_spPassData](GraphicsEngine* a_pGE, RenderContext* a_pCtx, uint8_t a_passIndex)
		{
			const auto& _srcNTexHandle = _spPassData->pRG->GetTexHandle("GBufferNormal");
			const auto& _dstNTexHandle = _spPassData->pRG->GetTexHandle("PrevNormal");

			const auto& _srcDTexHandle = _spPassData->pRG->GetTexHandle("Depth");
			const auto& _dstDTexHandle = _spPassData->pRG->GetTexHandle("PrevDepth");

			a_pCtx->TexCopy(_srcDTexHandle, _dstDTexHandle);
			a_pCtx->TexCopy(_srcNTexHandle, _dstNTexHandle);
		};

		a_rg.AddPassNode(a_phase, _node);
	}
}