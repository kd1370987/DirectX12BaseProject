#include "AttachmentSlotLinkSystem.h"

#include "Engine/ECS/World/World.h"

#include "../../../../Components/Charactor/Robot/AttachmentSlotsComponent.h"

void AttachmentSlotLinkSystem::Init(Engine::ECS::World& a_world)
{
	a_world.AwekeTask<AttachmentSlotsComponent>(
		Engine::ECS::ESystemType::PostDeserialize,
		"AttachmentSlotLinkSystem",
		[&a_world](
			Engine::ECS::ArchetypeChunk* a_pChunk,
			uint32_t a_count,
			float a_dt,
			AwekeTag* a_tag,
			AttachmentSlotsComponent* a_slotsArray
			)
		{
			// 1スロットの GUID -> id を解決
			auto _resolve = [&a_world](AttachmentSlot& a_slot)
			{
				if (a_slot.guid != Engine::DefaultGUID)
				{
					a_slot.id = a_world.GetEntity(a_slot.guid);
				}
				else
				{
					a_slot.id = Engine::ECS::Limits::INVALID_ENTITY;
				}
			};

			for (size_t _i = 0; _i < a_count; ++_i)
			{
				AttachmentSlotsComponent& _slots = a_slotsArray[_i];

				_resolve(_slots.rightShoulderBoost);
				_resolve(_slots.leftShoulderBoost);
				_resolve(_slots.rightLegBoost);
				_resolve(_slots.leftLegBoost);
				_resolve(_slots.mainGun);
				_resolve(_slots.missile);
			}
		}
	);
}
