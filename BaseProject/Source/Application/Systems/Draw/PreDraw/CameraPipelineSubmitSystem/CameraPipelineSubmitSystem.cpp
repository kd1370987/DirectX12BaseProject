#include "CameraPipelineSubmitSystem.h"

#include "Application/ECS/World/World.h"
#include "Engine/MainEngine.h"

#include "Application/Components/Tag/CameraTag.h"
#include "Application/Components/Transform/WorldMatrixComponent.h"
#include "Application/Components/Camera/CameraParamComponent.h"
#include "Application/Components/Camera/ProjMatComponent.h"

#include "Application/InstanceResource/SingletonEntityResource.h"

#include "Engine/Graphics/GraphicEngine.h"

//==========================================================================================
// CameraPipelineSubmitSystem
//
// 描画構成を持つカメラを、すべて GraphicsEngine へ送る。
//
// CamSetShaderSystem がメインカメラ1台ぶんを従来経路へ送るのに対し、
// こちらは「パイプラインを持つカメラ全部」を送る。
// 画面へ出ないサブカメラ・モニター用カメラもここを通る。
//
// 積まれなかったカメラは GraphicsEngine 側でフレームの終わりに捨てられるので、
// 毎フレーム送り直すこと。
//==========================================================================================
void CameraPipelineSubmitSystem::Init(App::ECS::World& a_world)
{
	a_world.ActiveCustomTask(
		Engine::ECS::ESystemType::PreDraw,
		Engine::ECS::ReadList<CameraTag, CameraParamComponent, ProjMatComponent, WorldMatrixComponent>{},
		Engine::ECS::WriteList<>{},
		[](const Engine::ECS::SystemContext& a_ctx)
		{
			if (!a_ctx.pWorld) return;
			if (!a_ctx.pServices || !a_ctx.pServices->pMainEngine) return;

			auto* _pGE = a_ctx.pServices->pMainEngine->RefGraphicsEngine();
			if (!_pGE) return;

			// 画面に出るカメラがどれかは MainCameraSystem が決めてある
			Engine::ECS::Entity _mainCamera = Engine::ECS::Limits::INVALID_ENTITY;
			if (a_ctx.pWorld->HasResource<SingletonEntityResource>())
			{
				_mainCamera = a_ctx.pWorld->GetResource<SingletonEntityResource>().mainCamera;
			}

			a_ctx.pWorld->ForEach<const ActiveTag, const CameraTag, const CameraParamComponent, const ProjMatComponent, const WorldMatrixComponent>(
				[&](
					Engine::ECS::ArchetypeChunk*		a_pChunk,
					uint32_t							a_count,
					const ActiveTag*					a_tags,
					const CameraTag*					a_camTagArray,
					const CameraParamComponent*			a_camParamArray,
					const ProjMatComponent*				a_projMatArray,
					const WorldMatrixComponent*			a_worldMatArray
				)
				{
					for (uint32_t _i = 0; _i < a_count; ++_i)
					{
						const CameraParamComponent& _param = a_camParamArray[_i];

						// 描画構成を持たないカメラは新経路に乗らない。
						// 従来のレンダーグラフだけが動く
						if (!_param.pipelineHandle.IsValid()) continue;

						const Engine::ECS::Entity _entity = a_pChunk->entityData[_i];

						Engine::Graphics::CameraSubmitDesc _desc = {};
						_desc.pWorld			= a_ctx.pWorld;
						_desc.entity			= static_cast<uint32_t>(_entity);
						_desc.pipelineHandle	= _param.pipelineHandle;
						_desc.worldMat			= a_worldMatArray[_i].worldMat;
						_desc.projMat			= a_projMatArray[_i].projMat;
						_desc.viewportWidth		= _param.viewportWidth;
						_desc.viewportHeight	= _param.viewportHeight;
						_desc.order				= _param.renderOrder;
						_desc.isMain			= (_entity == _mainCamera);

						_pGE->SubmitCamera(_desc);
					}
				}
			);
		}
	);
}
