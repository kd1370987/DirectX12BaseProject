#include "AttachmentDispatchSystem.h"

#include "Engine/ECS/World/World.h"

#include "../../../../Components/Charactor/Robot/AttachmentSlotsComponent.h"
#include "../../../../Components/Intent/ActionIntentComponent.h"

//==========================================================================================
// AttachmentDispatchSystem
//
// プレイヤー(AttachmentSlotsComponent 保持者)を反復し、スロットが指す子エンティティの
// コンポーネントへ入力を配信する。子エンティティはこのクエリには含まれないため、
// World::RefData でエンティティ横断的に参照する(構造変更は行わないので反復中でも安全)。
//
// ※ ブースター(移動スラスター)の噴射制御は ThrusterEffectSystem が担当する。
//    ここでは武器系の入力配信のみを行う。
//==========================================================================================
void AttachmentDispatchSystem::Init(Engine::ECS::World& a_world)
{
	a_world.ActiveTask<const AttachmentSlotsComponent, const ActionIntentComponent>(
		Engine::ECS::ESystemType::PreUpdate,
		"AttachmentDispatchSystem",
		[&a_world]
		(
			Engine::ECS::ArchetypeChunk* a_pChunk,
			uint32_t a_count,
			float a_dt,
			ActiveTag* a_tags,
			const AttachmentSlotsComponent* a_slotsArray,
			const ActionIntentComponent* a_intentArray
			)
		{
			// 銃子へ発射入力を配信
			auto _setGunIntent = [&a_world](Engine::ECS::Entity a_e, bool a_shoot, bool a_aim)
			{
				if (a_e == Engine::ECS::Limits::INVALID_ENTITY) return;
				if (!a_world.HasComponent<ActionIntentComponent>(a_e)) return;
				if (auto* _p = a_world.RefData<ActionIntentComponent>(a_e))
				{
					_p->isGunShoot = a_shoot;
					_p->isAiming = a_aim;
				}
			};

			for (size_t _i = 0; _i < a_count; ++_i)
			{
				const AttachmentSlotsComponent& _slots = a_slotsArray[_i];
				const ActionIntentComponent& _intent = a_intentArray[_i];

				// --- 銃スロットへ配信 ---
				_setGunIntent(_slots.mainGun.id, _intent.isGunShoot, _intent.isAiming);

				// TODO: missile スロットは専用入力ができ次第、同様に配信する
			}
		}
	);
}
