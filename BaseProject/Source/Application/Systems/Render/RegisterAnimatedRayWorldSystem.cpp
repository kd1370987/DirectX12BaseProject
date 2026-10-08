#include "RegisterAnimatedRayWorldSystem.h"
#include "Application/ECS/World/APPWorld.h"

#include "Engine/Graphics/Raytracing/RayEngine.h"

#include "Application/Components/Render/RayTag.h"
#include "Application/Components/Render/ModelComponent.h"
#include "Application/Components/Transform/WorldMatrixComponent.h"
#include "Application/Components/Animation/SkeletonPoseComponent.h"
#include "Application/Components/Render/DynamicRaytracingComponent.h"
#include "Application/Components/Animation/NodePoseComponent.h"

#include "Engine/MainEngine.h"
#include "Engine/Graphics/GraphicsEngine.h"
namespace App::System
{
	void RegisterAnimatedRayWorldSystem::Init(App::ECS::APPWorld& a_world)
	{
		a_world.ActiveTask<
			const Component::RayTag,
			const Component::ModelComponent,
			const Component::WorldMatrixComponent,
			const Component::DynamicRaytracingComponent,
			const Component::NodePoseComponent,
			const Component::SkeletonPoseComponent
		>
			(
				Engine::ECS::ESystemType::Draw,
				"RegisterAnimatedRayWorldSystem",
				[]
				(
					Engine::ECS::Chunk* a_pChunk,
					uint32_t a_count,
					const Engine::ECS::SystemContext& a_ctx,
					Component::ActiveTag* a_pTags,
					const Component::RayTag* a_pRayTags,
					const Component::ModelComponent* a_pModelArray,
					const Component::WorldMatrixComponent* a_pWorldMatArray,
					const Component::DynamicRaytracingComponent* a_pAnimationArray,
					const Component::NodePoseComponent* a_nodePoseArray,
					const Component::SkeletonPoseComponent* a_skeletonArray
					)
				{
					for (size_t _i = 0; _i < a_count; ++_i)
					{
						const Component::WorldMatrixComponent& _wMatComp = a_pWorldMatArray[_i];
						const Component::ModelComponent& _modelComp = a_pModelArray[_i];
						const Component::DynamicRaytracingComponent& _rayComp = a_pAnimationArray[_i];
						const Component::NodePoseComponent& _nodePoseComp = a_nodePoseArray[_i];
						const Component::SkeletonPoseComponent& _skePoseComp = a_skeletonArray[_i];

						auto* _model = a_ctx.pServices->pResourceManager->Get(_modelComp.handle);;
						if (!_model) continue;

						// レイトレワールドに登録
						a_ctx.pServices->pRayEngine->RegisterSkinningModel(
							*a_ctx.pWorld,
							_wMatComp.worldMat,
							_modelComp.handle,
							_rayComp.dynamicInstanceHandle,
							_nodePoseComp.nodePoseHandle,
							_modelComp.colorScale,
							_modelComp.emissiveScale,
							_modelComp.GetEmissiveAdd()
						);
					}
				}
			);
	}
}
