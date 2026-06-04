#include "GBufferHistoryPass.h"

#include "../../../RenderContext/RenderContext.h"
#include "../../../RenderGraph/RenderGraph.h"

namespace Engine::Graphics
{
	void GBufferHistoryPass::Excute(GraphicsEngine* a_pGE, RenderContext* a_pCtx)
	{
		const auto& _srcNTexHandle = m_pRG->GetTexHandle("GBufferNormal");
		const auto& _dstNTexHandle = m_pRG->GetTexHandle("PrevNormal");

		const auto& _srcDTexHandle = m_pRG->GetTexHandle("Depth");
		const auto& _dstDTexHandle = m_pRG->GetTexHandle("PrevDepth");

		a_pCtx->TexCopy(_srcDTexHandle,_dstDTexHandle);
		a_pCtx->TexCopy(_srcNTexHandle,_dstNTexHandle);
	}

	void GBufferHistoryPass::CreatePass()
	{
		AddRead("GBufferNormal", AccessType::CopySrc, LoadOp::Load, StoreOp::Store);
		AddRead("Depth", AccessType::CopySrc, LoadOp::Load, StoreOp::Store);

		AddWrite("PrevDepth", AccessType::CopyDst, LoadOp::Clear, StoreOp::Store);
		AddWrite("PrevNormal", AccessType::CopyDst, LoadOp::Clear, StoreOp::Store);
	}
}