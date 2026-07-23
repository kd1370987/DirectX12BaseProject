#include "RegisterPrevWorldMatSystem.h"

#include "Engine/ECS/World/World.h"
#include "Application/Components/Transform/WorldMatrixComponent.h"
#include "Application/Components/Transform/PreviousWorldMatrixComponent.h"

#include "Engine/Graphics/RenderContext/RenderContext.h"
#include "Engine/Graphics/RenderContext/ShapeDraw/ShapeDraw.h"

#include "Engine/Resource/Manager/ResourceManager/ResourceManager.h"
#include "Application/Components/Resource/AnimatorComponent.h"

void RegisterPrevWorldMatSystem::Init(Engine::ECS::World& a_world)
{
	a_world.ActiveTask<const WorldMatrixComponent, PreviousWorldMatrixComponent>(
		Engine::ECS::ESystemType::PostDraw,
		"RegisterPrevWorldMatSystem",
		[]
		(
			Engine::ECS::ArchetypeChunk* a_pChunk,
			uint32_t a_count,
			const Engine::ECS::SystemContext& a_ctx,
			ActiveTag* a_tags,
			const WorldMatrixComponent* a_worldMatArray,
			PreviousWorldMatrixComponent* a_prevWorldMatArray
			)
		{
			for (uint32_t _i = 0; _i < a_count; ++_i)
			{
				const auto& _worldMatComp = a_worldMatArray[_i];
				auto& _prevWorldMatComp = a_prevWorldMatArray[_i];

				_prevWorldMatComp.worldMat = _worldMatComp.worldMat;
			}
		}
	);
}
