#include "AttachmentDispatchSystem.h"

#include "Engine/ECS/World/World.h"

#include "../../../../Components/Charactor/Robot/AttachmentSlotsComponent.h"
#include "../../../../Components/Intent/ActionIntentComponent.h"
#include "../../../../Components/Charactor/AimTargetPosComponent.h"

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
			// 銃子へ発射入力を配信
			auto _setGunIntent = [&a_ctx](Engine::ECS::Entity a_e, bool a_shoot, bool a_aim)
			{
				if (a_e == Engine::ECS::Limits::INVALID_ENTITY) return;
				if (!a_ctx.pWorld->HasComponent<ActionIntentComponent>(a_e)) return;
				if (auto* _p = a_ctx.pWorld->RefData<ActionIntentComponent>(a_e))
				{
					_p->isGunShoot = a_shoot;
					_p->isAiming = a_aim;
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

				// --- 銃スロットへ配信 ---
				_setGunIntent(_slots.mainGun.id, _intent.isGunShoot, _intent.isAiming);
				_setAimTarget(_slots.mainGun.id, _pAim);

				// TODO: missile スロットは専用入力ができ次第、同様に配信する
			}
		}
	);
}
