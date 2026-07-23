#include "RegisterAnimatedRayWorldSystem.h"
#include "Engine/ECS/World/World.h"

#include "Engine/Raytracing/RaytracingEngine/RaytracingEngine.h"

#include "../../../../Components/Tag/RenderTag/RayTag.h"
#include "../../../../Components/Resource/ModelComponent.h"
#include "../../../../Components/Transform/WorldMatrixComponent.h"
#include "../../../../Components/Resource/SkeletonPoseComponent.h"
#include "Application/Components/Resource/AnimatorComponent.h"
#include "Application/Components/Resource/NodePoseComponent.h"

#include "../../../../../Engine/MainEngine.h"
#include "../../../../../Engine/Graphics/GraphicEngine.h"
void RegisterAnimatedRayWorldSystem::Init(Engine::ECS::World& a_world)
{
	a_world.ActiveTask<
		const RayTag,
		const ModelComponent,
		const WorldMatrixComponent,
		const AnimatorComponent,
		const NodePoseComponent,
		const SkeletonPoseComponent
	>
		(
			Engine::ECS::ESystemType::Draw,
			"RegisterRayWorldSystem",
			[]
			(
				Engine::ECS::ArchetypeChunk* a_pChunk,
				uint32_t a_count,
				const Engine::ECS::SystemContext& a_ctx,
				ActiveTag* a_pTags,
				const RayTag* a_pRayTags,
				const ModelComponent* a_pModelArray,
				const WorldMatrixComponent* a_pWorldMatArray,
				const AnimatorComponent* a_pAnimationArray,
				const NodePoseComponent* a_nodePoseArray,
				const SkeletonPoseComponent* a_skeletonArray
				)
			{
				for (size_t _i = 0; _i < a_count; ++_i)
				{
					const WorldMatrixComponent& _wMatComp = a_pWorldMatArray[_i];
					const ModelComponent& _modelComp = a_pModelArray[_i];
					const AnimatorComponent& _animComp = a_pAnimationArray[_i];
					const NodePoseComponent& _nodePoseComp = a_nodePoseArray[_i];
					const SkeletonPoseComponent& _skePoseComp = a_skeletonArray[_i];

					auto* _model = a_ctx.pServices->pResourceManager->Get(_modelComp.handle);;
					if (!_model) continue;

					// レイトレワールドに登録
					a_ctx.pServices->pRayEngine->RegisterSkinningModel(
						*a_ctx.pWorld,
						_wMatComp.worldMat,
						_modelComp.handle,
						_animComp.dynamicInstanceHandle,
						_nodePoseComp.nodePoseHandle,
						_modelComp.colorScale,
						_modelComp.emissiveScale
					);
				}
			}
		);
}
