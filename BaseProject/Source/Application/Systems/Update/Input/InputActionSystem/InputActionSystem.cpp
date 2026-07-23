#include "InputActionSystem.h"
#include "Engine/ECS/World/World.h"

#include "../../../../Components/Intent/ActionIntentComponent.h"
#include "../../../../Components/Tag/PlayerControllTag.h"


void InputActionSystem::Init(Engine::ECS::World& a_world)
{
	a_world.ActiveTask<const PlayerControllTag, ActionIntentComponent>(
		Engine::ECS::ESystemType::Input,
		"InputActionSystem",
		[]
		(
			Engine::ECS::ArchetypeChunk* a_pChunk,
			uint32_t a_count,
			const Engine::ECS::SystemContext& a_ctx,
			ActiveTag* a_ActiveTag,
			const PlayerControllTag* a_tags,
			ActionIntentComponent* a_actionIntentArray
			)
		{
			// ブースト
			bool _isShoot = Engine::Input::InputManager::Instance().IsHold("Shoot");
			bool _isAiming = Engine::Input::InputManager::Instance().IsHold("Aim");


			for (size_t _i = 0; _i < a_count; ++_i)
			{
				ActionIntentComponent& _intent = a_actionIntentArray[_i];
				_intent.isGunShoot = _isShoot;
				_intent.isAiming = _isAiming;
			}
		}
	);
}
