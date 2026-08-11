#include "EnemyShootIntentSystem.h"

#include "Engine/ECS/World/World.h"

#include "../../../../Components/Tag/EnemyTag.h"
#include "../../../../Components/Character/TargetEntityComponent.h"
#include "../../../../Components/Intent/ActionIntentComponent.h"

//==============================================================================
// EnemyShootIntentSystem
//
// 敵の発射入力を作る。プレイヤー側の InputActionSystem(キー入力 → Intent)と
// 同じ役割を、索敵結果(SearchPlayerSystem が書く isFind)で置き換えたもの。
//
// ・「見えている間は撃つ」だけの単純な判定。撃つ間隔(連射レート)や単発/フルオートは
//   GunStateComponent 側の設定に任せる。
// ・書き込み先は自分の ActionIntentComponent。銃を子エンティティとして持つ場合は
//   AttachmentDispatchSystem が同じ PreUpdate 帯で子へ配信する
//   (このシステムが ActionIntent を書き、あちらが読むので実行順は自動で決まる)。
// ・敵本体が直接 GunStateComponent を持っていてもよい。その場合は配信を挟まず、
//   同じ ActionIntentComponent を GunShootSystem がそのまま見る。
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

				// 見えている間だけ撃つ。見失ったらその時点で止める
				_intent.isGunShoot =
					_target.isFind &&
					_target.targetEntity != Engine::ECS::Limits::INVALID_ENTITY;
			}
		}
	);
}
