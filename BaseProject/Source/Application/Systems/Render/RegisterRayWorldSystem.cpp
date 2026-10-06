#include "RegisterRayWorldSystem.h"

#include "Application/ECS/World/APPWorld.h"

#include "Engine/Graphics/Raytracing/RaytracingEngine/RaytracingEngine.h"

#include "Application/Components/Render/RayTag.h"
#include "Application/Components/Render/ModelComponent.h"
#include "Application/Components/Transform/WorldMatrixComponent.h"
#include "Application/Components/Animation/AnimatorComponent.h"

namespace App::System
{
	void RegisterRayWorldSystem::Init(App::ECS::APPWorld& a_world)
	{
		a_world.ActiveTask<const Component::RayTag,const Component::ModelComponent,const Component::WorldMatrixComponent>(
			Engine::ECS::ESystemType::Draw,
			"RegisterRayWorldSystem",
			[]
			(
				Engine::ECS::Chunk* a_pChunk,
				uint32_t a_count,
				const Engine::ECS::SystemContext& a_ctx,
				Component::ActiveTag* a_pTags,
				const Component::RayTag* a_pRayTags,
				const Component::ModelComponent* a_pModelArray,
				const Component::WorldMatrixComponent* a_pWorldMatArray
			)
			{
				for (size_t _i = 0; _i < a_count; ++_i)
				{
					const Component::WorldMatrixComponent& _wMatComp = a_pWorldMatArray[_i];
					const Component::ModelComponent& _modelComp = a_pModelArray[_i];

					// レイトレワールドに記録
					a_ctx.pServices->pRayEngine->RegisterModel(
						_wMatComp.worldMat,
						_modelComp.handle,
						_modelComp.colorScale,
						_modelComp.emissiveScale,
						_modelComp.GetEmissiveAdd()
					);
				}
			},
			Engine::ECS::Exclude<Component::AnimatorComponent>()
		);
	}
}
