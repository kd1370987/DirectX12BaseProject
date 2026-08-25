#include "CamSetShaderSystem.h"

#include "Engine/ECS/World/World.h"
#include "Engine/MainEngine.h"

#include "Application/Components/Tag/CameraTag.h"

#include "Application/Components/Transform/WorldMatrixComponent.h"

#include "Application/Components/Camera/CameraParamComponent.h"
#include "Application/Components/Camera/FocusParamComponent.h"
#include "Application/Components/Camera/ProjMatComponent.h"

#include "Application/InstanceResource/SingletonEntityResource.h"

#include "Engine/Graphics/RenderContext/RenderContext.h"
#include "../../../../../Engine/Graphics/GraphicEngine.h"

//==========================================================================================
// CamSetShaderSystem
//
// メインカメラのビュー/射影行列(と被写界深度の設定)を GraphicsEngine へ送る。
//
// どのカメラで映すかは MainCameraSystem が決めて
// SingletonEntityResource.mainCamera に置いてあるので、ここでは探さずに引くだけ。
//==========================================================================================
void CamSetShaderSystem::Init(Engine::ECS::World& a_world)
{
	a_world.ActiveCustomTask(
		Engine::ECS::ESystemType::PreDraw,
		Engine::ECS::ReadList<CameraTag, ProjMatComponent, WorldMatrixComponent>{},
		Engine::ECS::WriteList<>{},
		[](const Engine::ECS::SystemContext& a_ctx)
		{
			if (!a_ctx.pWorld) return;
			if (!a_ctx.pWorld->HasResource<SingletonEntityResource>()) return;

			const auto& _singleton = a_ctx.pWorld->GetResource<SingletonEntityResource>();

			const Engine::ECS::Entity _camera = _singleton.mainCamera;
			if (_camera == Engine::ECS::Limits::INVALID_ENTITY) return;
			if (!a_ctx.pWorld->IsAliveEntity(_camera)) return;

			// 行列が無いカメラは送りようがない
			if (!a_ctx.pWorld->HasComponent<ProjMatComponent>(_camera))		return;
			if (!a_ctx.pWorld->HasComponent<WorldMatrixComponent>(_camera))	return;

			const auto* _pProjMat	= a_ctx.pWorld->RefData<ProjMatComponent>(_camera);
			const auto* _pWorldMat	= a_ctx.pWorld->RefData<WorldMatrixComponent>(_camera);
			if (!_pProjMat || !_pWorldMat) return;

			auto* _pGE = a_ctx.pServices->pMainEngine->RefGraphicsEngine();

			_pGE->SetCameraMat(_pWorldMat->worldMat);
			_pGE->SetProjMat(_pProjMat->projMat);

			//============================================================
			// 被写界深度(DoF)
			//------------------------------------------------------------
			// ピントはカメラの持ち物なので FocusParamComponent から送る。
			// 持っていないカメラもあるので、あるときだけ送る。
			// (無いことを理由に打ち切ると、ピント設定を持たないカメラが
			//  ビュー/射影行列ごと設定されなくなってしまう)
			//
			// 未設定のフレームは GraphicsEngine 側でリセット済み = ボケなし。
			//============================================================
			if (!a_ctx.pWorld->HasComponent<FocusParamComponent>(_camera)) return;

			const auto* _pFocus = a_ctx.pWorld->RefData<FocusParamComponent>(_camera);
			if (!_pFocus) return;

			Engine::Graphics::DoFOptionCB _dofCB = {};
			_dofCB.focusDistance	= _pFocus->focusDistance;
			_dofCB.focusRange		= _pFocus->focusRange;
			_dofCB.nearRange		= _pFocus->nearRange;
			_dofCB.farRange			= _pFocus->farRange;
			_dofCB.maxBlurRadius	= _pFocus->maxBlurRadius;
			_dofCB.enable			= _pFocus->enable ? 1 : 0;
			_pGE->SetDoFData(_dofCB);
		}
	);
}
