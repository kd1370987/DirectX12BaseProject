#include "RegisterAnimatedRayWorldSystem.h"
#include "Application/ECS/World/APPWorld.h"

#include "Engine/Graphics/Raytracing/RaytracingEngine/RaytracingEngine.h"

#include "Application/Components/Render/RayTag.h"
#include "Application/Components/Render/ModelComponent.h"
#include "Application/Components/Transform/WorldMatrixComponent.h"
#include "Application/Components/Animation/SkeletonPoseComponent.h"
#include "Application/Components/Render/DynamicRaytracingComponent.h"
#include "Application/Components/Animation/NodePoseComponent.h"

#include "Engine/MainEngine.h"
#include "Engine/Graphics/GraphicsEngine.h"
void RegisterAnimatedRayWorldSystem::Init(App::ECS::APPWorld& a_world)
{
	a_world.ActiveTask<
		const RayTag,
		const ModelComponent,
		const WorldMatrixComponent,
		const DynamicRaytracingComponent,
		const NodePoseComponent,
		const SkeletonPoseComponent
	>
		(
			Engine::ECS::ESystemType::Draw,
			"RegisterAnimatedRayWorldSystem",
			[]
			(
				Engine::ECS::Chunk* a_pChunk,
				uint32_t a_count,
				const Engine::ECS::SystemContext& a_ctx,
				ActiveTag* a_pTags,
				const RayTag* a_pRayTags,
				const ModelComponent* a_pModelArray,
				const WorldMatrixComponent* a_pWorldMatArray,
				const DynamicRaytracingComponent* a_pAnimationArray,
				const NodePoseComponent* a_nodePoseArray,
				const SkeletonPoseComponent* a_skeletonArray
				)
			{
				for (size_t _i = 0; _i < a_count; ++_i)
				{
					const WorldMatrixComponent& _wMatComp = a_pWorldMatArray[_i];
					const ModelComponent& _modelComp = a_pModelArray[_i];
					const DynamicRaytracingComponent& _rayComp = a_pAnimationArray[_i];
					const NodePoseComponent& _nodePoseComp = a_nodePoseArray[_i];
					const SkeletonPoseComponent& _skePoseComp = a_skeletonArray[_i];

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
