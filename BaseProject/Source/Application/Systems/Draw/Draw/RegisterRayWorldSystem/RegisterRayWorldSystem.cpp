#include "RegisterRayWorldSystem.h"

#include "Engine/ECS/World/World.h"

#include "Engine/Raytracing/RaytracingEngine/RaytracingEngine.h"

#include "../../../../Components/Tag/RenderTag/RayTag.h"
#include "../../../../Components/Resource/ModelComponent.h"
#include "../../../../Components/Transform/WorldMatrixComponent.h"
#include "Application/Components/Resource/AnimatorComponent.h"

void RegisterRayWorldSystem::Init(Engine::ECS::World& a_world)
{
	a_world.ActiveTask<const RayTag,const ModelComponent,const WorldMatrixComponent>(
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
			const WorldMatrixComponent* a_pWorldMatArray
		)
		{
			for (size_t _i = 0; _i < a_count; ++_i)
			{
				const WorldMatrixComponent& _wMatComp = a_pWorldMatArray[_i];
				const ModelComponent& _modelComp = a_pModelArray[_i];

				// レイトレワールドに記録
				Engine::Raytracing::RayEngine::Instance().RegistModel(
					_wMatComp.worldMat,
					_modelComp.handle,
					_modelComp.colorScale,
					_modelComp.emissiveScale
				);
			}
		},
		Engine::ECS::Exclude<AnimatorComponent>()
	);
}
