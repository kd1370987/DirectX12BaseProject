#include "OffScreenPass.h"

#include "Engine/Graphics/RenderContext/RenderContext.h"

void OffScreenPass::Begin(RenderContext* a_pCtx)
{
	a_pCtx->SetRootSig(m_passDesc.rootSigID);
	a_pCtx->SetGraphicPSO(m_passDesc.psoID);

	a_pCtx->BindCameraCB();
}

void OffScreenPass::End(RenderContext* a_pCtx)
{
}
