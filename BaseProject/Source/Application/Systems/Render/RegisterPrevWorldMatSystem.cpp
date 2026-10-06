#include "RegisterPrevWorldMatSystem.h"

#include "Application/ECS/World/APPWorld.h"
#include "Application/Components/Transform/WorldMatrixComponent.h"
#include "Application/Components/Transform/PreviousWorldMatrixComponent.h"

#include "Engine/Graphics/Frame/RenderContext/RenderContext.h"

#include "Engine/Resource/Manager/ResourceManager/ResourceManager.h"
#include "Application/Components/Animation/AnimatorComponent.h"

namespace App::System
{
	void RegisterPrevWorldMatSystem::Init(App::ECS::APPWorld& a_world)
	{
		// 自分のチャンクの行列を写すだけ(ボイド数千体ぶん)なので、ワーカーで回す
		a_world.ActiveJobTask<const Component::WorldMatrixComponent, Component::PreviousWorldMatrixComponent>(
			Engine::ECS::ESystemType::PostDraw,
			"RegisterPrevWorldMatSystem",
			[]
			(
				Engine::ECS::Chunk* a_pChunk,
				uint32_t a_count,
				const Engine::ECS::SystemContext& a_ctx,
				Component::ActiveTag* a_tags,
				const Component::WorldMatrixComponent* a_worldMatArray,
				Component::PreviousWorldMatrixComponent* a_prevWorldMatArray
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
}
