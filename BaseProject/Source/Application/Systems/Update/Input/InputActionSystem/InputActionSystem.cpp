#include "InputActionSystem.h"
#include "Application/ECS/World/World.h"

#include "../../../../Components/Intent/ActionIntentComponent.h"
#include "../../../../Components/Tag/PlayerControllTag.h"


void InputActionSystem::Init(App::ECS::World& a_world)
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
			// 武器 : 左クリックが左手、右クリックが右手。
			// ここで作るのは「押されている」という命令だけで、
			// 撃てるかどうか(連射間隔・バースト・熱)は武器側が決める
			bool _isLeftShoot = a_ctx.pServices->pInputManager->IsHold("ShootLeft");
			bool _isRightShoot = a_ctx.pServices->pInputManager->IsHold("ShootRight");

			// ミサイル : 押している間が溜め、離した瞬間が発射(判定は MissileSalvoSystem)
			bool _isMissile = a_ctx.pServices->pInputManager->IsHold("Missile");

			for (size_t _i = 0; _i < a_count; ++_i)
			{
				ActionIntentComponent& _intent = a_actionIntentArray[_i];
				_intent.isLeftWeaponShoot = _isLeftShoot;
				_intent.isRightWeaponShoot = _isRightShoot;
				_intent.isMissileHold = _isMissile;
			}
		}
	);
}
