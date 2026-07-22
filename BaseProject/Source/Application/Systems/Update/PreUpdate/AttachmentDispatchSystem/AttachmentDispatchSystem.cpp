#include "AttachmentDispatchSystem.h"

#include "Engine/ECS/World/World.h"

#include "../../../../Components/Charactor/Robot/AttachmentSlotsComponent.h"
#include "../../../../Components/Charactor/Robot/BoostComponent.h"
#include "../../../../Components/Intent/ActionIntentComponent.h"
#include "../../../../Components/Resource/ParticlesComponent.h"

//==========================================================================================
// AttachmentDispatchSystem
//
// プレイヤー(AttachmentSlotsComponent 保持者)を反復し、スロットが指す子エンティティの
// コンポーネントへ状態を書き込む。子エンティティはこのクエリには含まれないため、
// World::RefData でエンティティ横断的に参照する(構造変更は行わないので反復中でも安全)。
//==========================================================================================
void AttachmentDispatchSystem::Init(Engine::ECS::World& a_world)
{
	a_world.ActiveTask<const AttachmentSlotsComponent, const ActionIntentComponent, const BoostComponent>(
		Engine::ECS::ESystemType::PreUpdate,
		"AttachmentDispatchSystem",
		[&a_world]
		(
			Engine::ECS::ArchetypeChunk* a_pChunk,
			uint32_t a_count,
			float a_dt,
			ActiveTag* a_tags,
			const AttachmentSlotsComponent* a_slotsArray,
			const ActionIntentComponent* a_intentArray,
			const BoostComponent* a_boostArray
			)
		{
			// ブースター子の噴射 ON/OFF を設定
			auto _setBoosterPlay = [&a_world](Engine::ECS::Entity a_e, bool a_on)
			{
				if (a_e == Engine::ECS::Limits::INVALID_ENTITY) return;
				if (!a_world.HasComponent<ParticlesComponent>(a_e)) return;
				if (auto* _p = a_world.RefData<ParticlesComponent>(a_e)) _p->isPlay = a_on;
			};

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
				const BoostComponent& _boost = a_boostArray[_i];

				// ブースト中か : 入力が入っていて、かつ燃料が使用量を上回っている
				// (RobotBoostSystem の推力適用条件に合わせている)
				bool _boostActive = _boost.isBoostIntent && (_boost.currentFuel > _boost.boostFuel);

				// --- ブースタースロットへ配信 ---
				_setBoosterPlay(_slots.rightShoulderBoost.id, _boostActive);
				_setBoosterPlay(_slots.leftShoulderBoost.id,  _boostActive);
				_setBoosterPlay(_slots.rightLegBoost.id,      _boostActive);
				_setBoosterPlay(_slots.leftLegBoost.id,       _boostActive);

				// --- 銃スロットへ配信 ---
				_setGunIntent(_slots.mainGun.id, _intent.isGunShoot, _intent.isAiming);

				// TODO: missile スロットは専用入力ができ次第、同様に配信する
			}
		}
	);
}
