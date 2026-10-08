#include "CameraPipelineSubmitSystem.h"

#include "Application/ECS/World/APPWorld.h"
#include "Engine/MainEngine.h"

#include "Application/Components/Camera/CameraTag.h"
#include "Application/Components/Transform/WorldMatrixComponent.h"
#include "Application/Components/Camera/CameraParamComponent.h"
#include "Application/Components/Camera/ProjMatComponent.h"

#include "Application/InstanceResource/SingletonEntityResource.h"

#include "Engine/Graphics/GraphicsEngine.h"

namespace App::System
{
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
	void CameraPipelineSubmitSystem::Init(App::ECS::APPWorld& a_world)
	{
		a_world.ActiveCustomTask(
			Engine::ECS::ESystemType::PreDraw,
			"CameraPipelineSubmitSystem",
			Engine::ECS::ReadList<Component::CameraTag, Component::CameraParamComponent, Component::ProjMatComponent, Component::WorldMatrixComponent>{},
			Engine::ECS::WriteList<>{},
			[](const Engine::ECS::SystemContext& a_ctx)
			{
				if (!a_ctx.pWorld) return;
				if (!a_ctx.pServices || !a_ctx.pServices->pMainEngine) return;

				auto* _pGE = a_ctx.pServices->pMainEngine->RefGraphicsEngine();
				if (!_pGE) return;

				// 画面に出るカメラがどれかは MainCameraSystem が決めてある
				Engine::ECS::Entity _mainCamera = Engine::ECS::Limits::INVALID_ENTITY;
				if (a_ctx.pWorld->HasResource<InstanceResource::SingletonEntityResource>())
				{
					_mainCamera = a_ctx.pWorld->RefResource<InstanceResource::SingletonEntityResource>().mainCamera;
				}

				a_ctx.pWorld->ForEach<const Component::ActiveTag, const Component::CameraTag, const Component::CameraParamComponent, const Component::ProjMatComponent, const Component::WorldMatrixComponent>(
					[&](
						Engine::ECS::Chunk*		a_pChunk,
						uint32_t							a_count,
						const Component::ActiveTag*					/*a_tags*/,
						const Component::CameraTag*					/*a_camTagArray*/,
						const Component::CameraParamComponent*			a_camParamArray,
						const Component::ProjMatComponent*				a_projMatArray,
						const Component::WorldMatrixComponent*			a_worldMatArray
					)
					{
						for (uint32_t _i = 0; _i < a_count; ++_i)
						{
							const Component::CameraParamComponent& _param = a_camParamArray[_i];

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

							_pGE->RefCameraPipelines()->SubmitCamera(_desc);
						}
					}
				);
			}
		)
		.ReadsResource<InstanceResource::SingletonEntityResource>();
	}
}
