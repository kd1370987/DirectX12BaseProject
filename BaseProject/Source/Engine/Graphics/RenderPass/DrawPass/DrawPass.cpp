#include "DrawPass.h"

#include "Engine/Graphics/RenderContext/RenderContext.h"

void DrawPass::DrawQueue(RenderContext* a_pCtx, RenderQueueType a_type)
{
	/*auto _draws = a_pCtx->GetDraw(a_type);
	for (auto& _draw : _draws)
	{
		a_pCtx->BindMaterial(_draw.pMaterial,_draw.colorScale,_draw.emissiveScale);
		a_pCtx->BindMesh(_draw.pMesh);
		a_pCtx->DrawPrimitive(_draw);
	}*/
}
