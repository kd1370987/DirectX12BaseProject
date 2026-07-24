#include "FollowTargetLinkSystem.h"

#include "Engine/ECS/World/World.h"

#include "../../../../Components/Persistence/GUIDComponent.h"
#include "../../../../Components/Camera/FollowTargetComponent.h""

void FollowTargetLinkSystem::Init(Engine::ECS::World& a_world)
{
	a_world.AwakeTask<const GUIDComponent,FollowTargetComponent>(
		Engine::ECS::ESystemType::PostDeserialize,
		"FollowTargetLinkSystem",
		[](
			Engine::ECS::ArchetypeChunk* a_pChunk,
			uint32_t a_count,
			const Engine::ECS::SystemContext& a_ctx,
			AwakeTag* a_tag,
			const GUIDComponent* a_guidArray,
			FollowTargetComponent* a_followArray
			)
		{
			for (size_t _i = 0; _i < a_count; ++_i)
			{
				FollowTargetComponent& _followComp = a_followArray[_i];

				// ターゲットGUIDがあるのなら
				if (_followComp.targetGUID != Engine::DefaultGUID)
				{
					_followComp.target = a_ctx.pWorld->GetEntity(_followComp.targetGUID);
				}
			}
		}
	);
}
