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
		RenderPassNode _node = {};
		_node.name = "GBufferHistoryPass";
		RGGlobalsPassBuilder _rpBuilder(&_node);

		_rpBuilder.CopySrc("GBufferNormal");
		_rpBuilder.CopySrc("Depth");

		_rpBuilder.CopyDst("PrevDepth", DXGI_FORMAT_R32_FLOAT);
		_rpBuilder.CopyDst("PrevNormal", DXGI_FORMAT_R16G16_FLOAT);


		_node.executeFunc = [](GraphicsEngine* a_pGE, RenderContext* a_pCtx, uint8_t a_passIndex)
		{
			const auto& _srcNTexHandle = a_pGE->RefRenderGraph()->GetTexHandle("GBufferNormal");
			const auto& _dstNTexHandle = a_pGE->RefRenderGraph()->GetTexHandle("PrevNormal");

			const auto& _srcDTexHandle = a_pGE->RefRenderGraph()->GetTexHandle("Depth");
			const auto& _dstDTexHandle = a_pGE->RefRenderGraph()->GetTexHandle("PrevDepth");

			a_pCtx->TexCopy(_srcDTexHandle, _dstDTexHandle);
			a_pCtx->TexCopy(_srcNTexHandle, _dstNTexHandle);
		};

		_node.phase = a_phase;
		a_pRegistry->RegisterPass(_node);
	}
}