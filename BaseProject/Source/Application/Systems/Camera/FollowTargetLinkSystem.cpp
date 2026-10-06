#include "FollowTargetLinkSystem.h"

#include "Application/ECS/World/APPWorld.h"

#include "Application/Components/Core/GUIDComponent.h"
#include "Application/Components/Camera/FollowTargetComponent.h"

namespace App::System
{
	void FollowTargetLinkSystem::Init(App::ECS::APPWorld& a_world)
	{
		a_world.AwakeTask<const Component::GUIDComponent,Component::FollowTargetComponent>(
			// AwakeTag を見るので Awake フェーズで回す
			Engine::ECS::ESystemType::Awake,
			"FollowTargetLinkSystem",
			[](
				Engine::ECS::Chunk* a_pChunk,
				uint32_t a_count,
				const Engine::ECS::SystemContext& a_ctx,
				Component::AwakeTag* a_tag,
				const Component::GUIDComponent* a_guidArray,
				Component::FollowTargetComponent* a_followArray
				)
			{
				for (size_t _i = 0; _i < a_count; ++_i)
				{
					Component::FollowTargetComponent& _followComp = a_followArray[_i];

					// ターゲットGUIDがあるのなら
					if (_followComp.targetGUID != Core::DEFAULT_GUID)
					{
						_followComp.target = a_ctx.pWorld->GetEntity(_followComp.targetGUID);
					}
				}
			}
		);
	}
}
