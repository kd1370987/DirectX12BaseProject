#include "AttachmentDispatchSystem.h"

#include "Application/ECS/World/World.h"

#include "../../../../Components/Character/Robot/AttachmentSlotsComponent.h"
#include "../../../../Components/Intent/ActionIntentComponent.h"
#include "../../../../Components/Character/AimTargetPosComponent.h"
#include "../../../../Components/Character/Weapon/WeaponTriggerComponent.h"

//==========================================================================================
// AttachmentDispatchSystem
//
// プレイヤー(AttachmentSlotsComponent 保持者)を反復し、スロットが指す子エンティティの
// コンポーネントへ入力を配信する。子エンティティはこのクエリには含まれないため、
// World::RefData でエンティティ横断的に参照する(構造変更は行わないので反復中でも安全)。
//
// ここが渡すのは「引き金を引いているか」と「どこを狙っているか」だけ。
// 撃てるかどうか(連射間隔・バースト・オーバーヒート)も、何をどう撃つか(弾・弾速・銃口)も
// 武器側の GunStateComponent が持ち、GunShootSystem が判断する。
// 持ち主は左右どちらの武器を使うかしか知らない。
//
// ※ ブースター(移動スラスター)の噴射制御は ThrusterEffectSystem が担当する。
//    ここでは武器系の入力配信のみを行う。
//==========================================================================================
void AttachmentDispatchSystem::Init(App::ECS::World& a_world)
{
	a_world.ActiveTask<const AttachmentSlotsComponent, const ActionIntentComponent>(
		Engine::ECS::ESystemType::PreUpdate,
		"AttachmentDispatchSystem",
		[]
		(
			Engine::ECS::ArchetypeChunk* a_pChunk,
			uint32_t a_count,
			const Engine::ECS::SystemContext& a_ctx,
			ActiveTag* a_tags,
			const AttachmentSlotsComponent* a_slotsArray,
			const ActionIntentComponent* a_intentArray
			)
		{
			// 武器へ引き金の状態を配信
			auto _setTrigger = [&a_ctx](Engine::ECS::Entity a_e, bool a_pulled)
			{
				if (a_e == Engine::ECS::Limits::INVALID_ENTITY) return;
				if (!a_ctx.pWorld->HasComponent<WeaponTriggerComponent>(a_e)) return;
				if (auto* _p = a_ctx.pWorld->RefData<WeaponTriggerComponent>(a_e))
				{
					_p->isPulled = a_pulled;
				}
			};

			// 狙点(AimTargetSystem がカメラのレイで求めた着弾点)を子へ配信
			auto _setAimTarget = [&a_ctx](Engine::ECS::Entity a_e, const AimTargetPosComponent* a_pSrc)
			{
				if (!a_pSrc) return;
				if (a_e == Engine::ECS::Limits::INVALID_ENTITY) return;
				if (!a_ctx.pWorld->HasComponent<AimTargetPosComponent>(a_e)) return;
				if (auto* _p = a_ctx.pWorld->RefData<AimTargetPosComponent>(a_e))
				{
					// 結果だけを渡す(maxDistance / startOffset は子側の設定を壊さない)
					_p->pos			= a_pSrc->pos;
					_p->dir			= a_pSrc->dir;
					_p->hitEntity	= a_pSrc->hitEntity;
					_p->isHit		= a_pSrc->isHit;
					_p->isValid		= a_pSrc->isValid;
				}
			};

			for (size_t _i = 0; _i < a_count; ++_i)
			{
				const AttachmentSlotsComponent& _slots = a_slotsArray[_i];
				const ActionIntentComponent& _intent = a_intentArray[_i];

				// 親の狙点(付いていないプレイヤーもあり得るので任意扱い)
				Engine::ECS::Entity _self = a_pChunk->entityData[_i];
				const AimTargetPosComponent* _pAim =
					a_ctx.pWorld->HasComponent<AimTargetPosComponent>(_self)
					? a_ctx.pWorld->RefData<AimTargetPosComponent>(_self)
					: nullptr;

				// --- 左右の武器へ配信 ---
				// スロットが空でも、片方だけでも、ここは何も気にしない。
				// 「押されている」と伝えるだけで、撃つかどうかは武器が決める
				_setTrigger(_slots.leftWeapon.id,  _intent.isLeftWeaponShoot);
				_setTrigger(_slots.rightWeapon.id, _intent.isRightWeaponShoot);

				_setAimTarget(_slots.leftWeapon.id,  _pAim);
				_setAimTarget(_slots.rightWeapon.id, _pAim);

				// missile スロットへは配信しない。ミサイルは「押している間に溜めて
				// 離すと一斉射」なので、入力を子へ流すだけでは足りない。
				// MissileSalvoSystem がプレイヤー側で溜めを持ち、発射のときだけ
				// このスロットが指すポッドの GunStateComponent(弾・弾速・銃口)を読む
			}
		}
	);
}
