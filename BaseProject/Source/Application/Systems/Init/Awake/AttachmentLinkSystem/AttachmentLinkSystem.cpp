#include "AttachmentLinkSystem.h"
#include "Engine/ECS/World/World.h"

#include "../../../../Components/Persistence/GUIDComponent.h"
#include "../../../../Components/Hierarchy/ExoskeletonAttachementComponent.h"

void AttachmentLinkSystem::Init(Engine::ECS::World& a_world)
{
	a_world.AwekeTask<const GUIDComponent, ExoskeletonAttachmentComponent>(
		Engine::ECS::ESystemType::PostDeserialize,
		[&a_world](
			Engine::ECS::ArchetypeChunk* a_pChunk,
			uint32_t a_count,
			float a_dt,
			AwekeTag* a_tag,
			const GUIDComponent* a_guidArray,
			ExoskeletonAttachmentComponent* a_followArray
			)
		{
			for (size_t _i = 0; _i < a_count; ++_i)
			{
				ExoskeletonAttachmentComponent& _followComp = a_followArray[_i];

				// ターゲットGUIDがあるのなら
				if (_followComp.parentGUID != Engine::DefaultGUID)
				{
					_followComp.parentID = a_world.GetEntity(_followComp.parentGUID);
				}
			}
		}
	);
}
