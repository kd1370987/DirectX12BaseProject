#include "DrawPass.h"

#include "Engine/Graphics/RenderContext/RenderContext.h"
namespace Engine::Graphics
{
	void DrawPass::DrawQueue(RenderContext* a_pCtx, RenderQueueType a_type)
	{
		//return;
		auto& _draws = a_pCtx->GetItemVec(a_type);
		if (_draws.size() == 0) return;
		for (auto& _item : _draws)
		{
			a_pCtx->BindObuje(
				{ 0.0f,0.0f },
				{ 1.0f,1.0f }
			);

			a_pCtx->BindMaterial(_item.pMaterial, _item.colorScale, _item.emissiveScale);
			a_pCtx->BindMesh(_item.pMesh, _item.worldMat);

			if (a_type == RenderQueueType::AnimationOpaque || a_type == RenderQueueType::AnimationTransparent)
			{
				a_pCtx->BindBone(
					_item.pBoneMatrices,
					_item.boneCount
				);
			}

			a_pCtx->Draw(_item.pMesh, _item.subIdx);
		}
	}
}