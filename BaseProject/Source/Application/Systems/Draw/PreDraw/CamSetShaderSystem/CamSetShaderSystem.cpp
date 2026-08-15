#include "CamSetShaderSystem.h"

#include "Engine/ECS/World/World.h"
#include "Engine/MainEngine.h"

#include "Application/Components/Tag/CameraTag.h"
#include "Application/Components/Tag/ActiveCameraTag.h"

#include "Application/Components/Transform/WorldMatrixComponent.h"

#include "Application/Components/Camera/CameraParamComponent.h"
#include "Application/Components/Camera/FocusParamComponent.h"
#include "Application/Components/Camera/ProjMatComponent.h"

#include "Engine/Graphics/RenderContext/RenderContext.h"
#include "../../../../../Engine/Graphics/GraphicEngine.h"

void CamSetShaderSystem::Init(Engine::ECS::World& a_world)
{
	a_world.ActiveTask<const ActiveCameraTag, const CameraTag, const ProjMatComponent, const WorldMatrixComponent>(
		Engine::ECS::ESystemType::PreDraw,
		"CamSetShaderSystem",
		[](
			Engine::ECS::ArchetypeChunk* a_pChunk,
			uint32_t a_count,
			const Engine::ECS::SystemContext& a_ctx,
			ActiveTag* a_tags,
			const ActiveCameraTag* a_activeCamTagArray,
			const CameraTag* a_camTagArray,
			const ProjMatComponent* a_projMatArray,
			const WorldMatrixComponent* a_worldMatArray
		)
		{
			auto* _pRCT = a_ctx.pServices->pMainEngine->RefRenderContext();
			auto* _pGE = a_ctx.pServices->pMainEngine->RefGraphicsEngine();

			for (size_t _i = 0; _i < a_count; ++_i)
			{
				const ProjMatComponent& _projMatComp = a_projMatArray[_i];
				const WorldMatrixComponent& _worldMatComp = a_worldMatArray[_i];

				_pGE->SetCameraMat(_worldMatComp.worldMat);
				_pGE->SetProjMat(_projMatComp.projMat);

				//============================================================
				// 被写界深度(DoF)
				//------------------------------------------------------------
				// ピントはカメラの持ち物なので FocusParamComponent から送る。
				// クエリ条件には入れず、持っているカメラだけ拾う。
				// (条件に足すと、ピント設定を持たないカメラが
				//  ビュー/射影行列ごと設定されなくなってしまう)
				//
				// 未設定のフレームは GraphicsEngine 側でリセット済み = ボケなし。
				//============================================================
				Engine::ECS::Entity _self = a_pChunk->entityData[_i];
				if (!a_ctx.pWorld->HasComponent<FocusParamComponent>(_self)) continue;

				const auto* _pFocus = a_ctx.pWorld->RefData<FocusParamComponent>(_self);
				if (!_pFocus) continue;

				Engine::Graphics::DoFOptionCB _dofCB = {};
				_dofCB.focusDistance	= _pFocus->focusDistance;
				_dofCB.focusRange		= _pFocus->focusRange;
				_dofCB.nearRange		= _pFocus->nearRange;
				_dofCB.farRange			= _pFocus->farRange;
				_dofCB.maxBlurRadius	= _pFocus->maxBlurRadius;
				_dofCB.enable			= _pFocus->enable ? 1 : 0;
				_pGE->SetDoFData(_dofCB);
			}
		}
	);
}