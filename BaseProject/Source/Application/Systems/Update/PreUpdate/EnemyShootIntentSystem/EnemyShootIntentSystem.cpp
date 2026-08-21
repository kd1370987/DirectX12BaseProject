#include "EnemyShootIntentSystem.h"

#include "Engine/ECS/World/World.h"

#include "../../../../Components/Tag/EnemyTag.h"
#include "../../../../Components/Character/TargetEntityComponent.h"
#include "../../../../Components/Character/Boss/BossComponent.h"
#include "../../../../Components/Intent/ActionIntentComponent.h"

//==============================================================================
// EnemyShootIntentSystem
//
// 敵の発射入力を作る。プレイヤー側の InputActionSystem(キー入力 → Intent)と
// 同じ役割を、索敵結果(SearchPlayerSystem が書く isInAttackRange)で置き換えたもの。
//
// ・撃つのは「攻撃可能距離の内側」だけ。発見しただけ(isFind)ではまだ撃たず、
//   EnemyMoveIntentSystem の追従で攻撃圏まで詰めてから撃ち始める。
//   距離の設定は TargetEntityComponent(detectDistance / attackDistance)。
// ・撃つ間隔(連射レート)や単発/フルオート、オーバーヒートは GunStateComponent 側の設定に任せる。
//   ここが作るのは「引き金を引きたいか」だけ。
// ・書き込み先は自分の ActionIntentComponent。銃を子エンティティとして持つ場合は
//   AttachmentDispatchSystem が同じ PreUpdate 帯で子の WeaponTriggerComponent へ配信する
//   (このシステムが ActionIntent を書き、あちらが読むので実行順は自動で決まる)。
// ・敵本体が直接 GunStateComponent を持っていてもよい。その場合は
//   SelfWeaponTriggerSystem が自分の WeaponTriggerComponent へ渡す。
// ・左右の区別は付けず両方に同じ値を入れる。片手しか持たない敵に
//   「どちらの手で撃つか」を決めさせても意味が無いため。
// ・ボスは除外する。ボスは距離ではなく戦闘開始命令で撃ち始め、撃つ/休むのリズムも
//   自分で持つので、BossCombatIntentSystem が同じ ActionIntentComponent を書く。
//   どちらも書き手になると実行順が登録順頼みになり、撃つ/撃たないが安定しない。
//==============================================================================
void EnemyShootIntentSystem::Init(Engine::ECS::World& a_world)
{
	a_world.ActiveTask<const EnemyTag, const TargetEntityComponent, ActionIntentComponent>(
		Engine::ECS::ESystemType::PreUpdate,
		"EnemyShootIntentSystem",
		[](
			Engine::ECS::ArchetypeChunk*      a_pChunk,
			uint32_t                          a_count,
			const Engine::ECS::SystemContext& a_ctx,
			ActiveTag*                        a_activeTags,
			const EnemyTag*                   a_enemyTags,
			const TargetEntityComponent*      a_targetArray,
			ActionIntentComponent*            a_intentArray
		)
		{
			for (size_t _i = 0; _i < a_count; ++_i)
			{
				const TargetEntityComponent& _target = a_targetArray[_i];
				ActionIntentComponent&       _intent = a_intentArray[_i];

				// 攻撃圏の内側にいる間だけ撃つ。離されたらその時点で止める
				const bool _shoot =
					_target.isInAttackRange &&
					_target.targetEntity != Engine::ECS::Limits::INVALID_ENTITY;

				_intent.isLeftWeaponShoot  = _shoot;
				_intent.isRightWeaponShoot = _shoot;
			}
		},
		Engine::ECS::Exclude<BossComponent>{}
	);
}
