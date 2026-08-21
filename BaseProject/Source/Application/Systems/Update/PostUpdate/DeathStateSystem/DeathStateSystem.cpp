#include "DeathStateSystem.h"

#include "Engine/ECS/World/World.h"

#include "Application/Components/Character/HealthComponent.h"
#include "Application/Components/Character/Boss/BossComponent.h"
#include "Application/Components/Character/Robot/BoostComponent.h"
#include "Application/Components/Intent/MoveIntentComponent.h"
#include "Application/Components/Intent/ActionIntentComponent.h"
#include "Application/Components/Resource/ActionStateComponent.h"

#include "Engine/Resource/Data/ActionStateMachineAsset/ActionStateMachineAsset.h"

//==============================================================================
// DeathStateSystem
//
// 「死んだ」から「消える」までの間を受け持つ。
//
// 体力が尽きた時点で消してしまうと、死亡を読む側(死亡エフェクトを出す
// DeathEffectSystem など)が本人のコンポーネントを引けない。
// そこで HealthSystem は死亡状態にするだけにして、実際に消すのはここが
// releaseDelay 秒あとに行う。そのあいだ死体は動かないようにする。
//
// タスクは分かれているが、どれも「死んでいるか」しか見ていない。
//
//   [PreUpdate] FSMへ IsDead を渡す
//       行動ステートマシンの Death ノードへ落とすためのパラメータ。
//       他の橋渡し(SightStateBridgeSystem 等)と同じ帯に置いてあるので、
//       遷移を評価する ActionStateCommitSystem(Update)より必ず先になる。
//       Death ノードを canMove=false にしておけば、水平速度は
//       ActionBehaviorSystem が止めてくれる(重力はそのまま = その場に落ちる)。
//
//   [Update] 入力/AIの結果を握りつぶす
//       意図を作るのは Input 帯(プレイヤー)と PreUpdate 帯(敵・ボス)なので、
//       Update 帯で消せば作り手がどれでも後から潰せる。
//       消費側(CharacterMovementSystem / GunShootSystem / BossMissileSalvoSystem)は
//       ここが書いたものを読む側になるため、依存の向きだけで自動的に後ろへ並ぶ。
//
//   [PostUpdate] 時間を進めて解放予約する
//       releaseDelay を過ぎたら AddReleaseEntity。解放予約したエンティティは
//       次の BeginFrame で ActiveTag が外れるので、このタスクは二度と当たらない。
//
// ※ 死亡状態そのものを別コンポーネント(DeadTag 等)にしなかったのは、
//   ランタイムの AddComponent が「アーキタイプの引っ越し + PostDeserialize からやり直し」に
//   なるため。初期化系(ActionStateFixupSystem など)が死ぬたびに走り直してしまう。
//==============================================================================
void DeathStateSystem::Init(Engine::ECS::World& a_world)
{
	//--------------------------------------------------------------------------
	// [PreUpdate] 死亡を行動ステートマシンへ渡す
	//--------------------------------------------------------------------------
	a_world.ActiveTask<const HealthComponent, ActionStateComponent>(
		Engine::ECS::ESystemType::PreUpdate,
		"DeathStateBridgeSystem",
		[](
			Engine::ECS::ArchetypeChunk*      a_pChunk,
			uint32_t                          a_count,
			const Engine::ECS::SystemContext& a_ctx,
			ActiveTag*                        a_tags,
			const HealthComponent*            a_healthArray,
			ActionStateComponent*             a_stateArray
		)
		{
			static const UINT s_deadHash = Engine::String::ToHash("IsDead");

			auto& _pool =
				a_ctx.pWorld->GetResource<Engine::Pool::ItemPool<Engine::Resource::ActionStateInstance>>();

			for (size_t _i = 0; _i < a_count; ++_i)
			{
				const HealthComponent& _health = a_healthArray[_i];
				ActionStateComponent&  _state  = a_stateArray[_i];

				auto* _pInstance = _pool.Ref(_state.instanceHandle);
				if (!_pInstance) continue;

				// 設計図(パラメータ定義を足すので Ref で可変参照を取る)
				auto* _pActionSM = a_ctx.pServices->pResourceManager->Ref(_state.actionHandle);
				if (!_pActionSM) continue;

				_pActionSM->SetBoolParam(*_pInstance, s_deadHash, "IsDead", _health.isDead);
			}
		}
	);

	//--------------------------------------------------------------------------
	// [Update] 死んでいるあいだの移動入力を消す
	//--------------------------------------------------------------------------
	a_world.ActiveTask<const HealthComponent, MoveIntentComponent>(
		Engine::ECS::ESystemType::Update,
		"DeathMoveIntentGateSystem",
		[](
			Engine::ECS::ArchetypeChunk*      a_pChunk,
			uint32_t                          a_count,
			const Engine::ECS::SystemContext& a_ctx,
			ActiveTag*                        a_tags,
			const HealthComponent*            a_healthArray,
			MoveIntentComponent*              a_intentArray
		)
		{
			for (size_t _i = 0; _i < a_count; ++_i)
			{
				if (!a_healthArray[_i].isDead) continue;

				MoveIntentComponent& _intent = a_intentArray[_i];
				_intent.value   = { 0.0f, 0.0f, 0.0f };
				_intent.jumpPow = 0.0f;
			}
		}
	);

	//--------------------------------------------------------------------------
	// [Update] 死んでいるあいだの攻撃入力を消す
	//--------------------------------------------------------------------------
	a_world.ActiveTask<const HealthComponent, ActionIntentComponent>(
		Engine::ECS::ESystemType::Update,
		"DeathActionIntentGateSystem",
		[](
			Engine::ECS::ArchetypeChunk*      a_pChunk,
			uint32_t                          a_count,
			const Engine::ECS::SystemContext& a_ctx,
			ActiveTag*                        a_tags,
			const HealthComponent*            a_healthArray,
			ActionIntentComponent*            a_intentArray
		)
		{
			for (size_t _i = 0; _i < a_count; ++_i)
			{
				if (!a_healthArray[_i].isDead) continue;

				ActionIntentComponent& _intent = a_intentArray[_i];
				_intent.isLeftWeaponShoot  = false;
				_intent.isRightWeaponShoot = false;
				_intent.isMissileHold      = false;
			}
		}
	);

	//--------------------------------------------------------------------------
	// [Update] 死んでいるあいだのブーストを止める
	//
	// ブーストは移動入力とは別系統(RobotBoostSystem が推力に変える)なので、
	// MoveIntent を消しただけでは飛び続けてしまう
	//--------------------------------------------------------------------------
	a_world.ActiveTask<const HealthComponent, BoostComponent>(
		Engine::ECS::ESystemType::Update,
		"DeathBoostGateSystem",
		[](
			Engine::ECS::ArchetypeChunk*      a_pChunk,
			uint32_t                          a_count,
			const Engine::ECS::SystemContext& a_ctx,
			ActiveTag*                        a_tags,
			const HealthComponent*            a_healthArray,
			BoostComponent*                   a_boostArray
		)
		{
			for (size_t _i = 0; _i < a_count; ++_i)
			{
				if (!a_healthArray[_i].isDead) continue;

				BoostComponent& _boost = a_boostArray[_i];
				_boost.isBoostTriger = false;
				_boost.isJustBoosted = false;
				_boost.isBoostIntent = false;
			}
		}
	);

	//--------------------------------------------------------------------------
	// [Update] 死んでいるあいだのボスの一斉射要求を消す
	//
	// ボスのミサイルは ActionIntent ではなく BossComponent 側の要求フラグで飛ぶ。
	// 消費するのは PostUpdate の BossMissileSalvoSystem
	//--------------------------------------------------------------------------
	a_world.ActiveTask<const HealthComponent, BossComponent>(
		Engine::ECS::ESystemType::Update,
		"DeathBossOrderGateSystem",
		[](
			Engine::ECS::ArchetypeChunk*      a_pChunk,
			uint32_t                          a_count,
			const Engine::ECS::SystemContext& a_ctx,
			ActiveTag*                        a_tags,
			const HealthComponent*            a_healthArray,
			BossComponent*                    a_bossArray
		)
		{
			for (size_t _i = 0; _i < a_count; ++_i)
			{
				if (!a_healthArray[_i].isDead) continue;

				BossComponent& _boss = a_bossArray[_i];
				_boss.isMissileRequest = false;
				_boss.isGunActive      = false;
			}
		}
	);

	//--------------------------------------------------------------------------
	// [PostUpdate] 死亡してからの時間を進め、尽きたら解放予約する
	//--------------------------------------------------------------------------
	a_world.ActiveTask<HealthComponent>(
		Engine::ECS::ESystemType::PostUpdate,
		"DeathReleaseSystem",
		[](
			Engine::ECS::ArchetypeChunk*      a_pChunk,
			uint32_t                          a_count,
			const Engine::ECS::SystemContext& a_ctx,
			ActiveTag*                        a_tags,
			HealthComponent*                  a_healthArray
		)
		{
			for (size_t _i = 0; _i < a_count; ++_i)
			{
				HealthComponent& _health = a_healthArray[_i];

				if (!_health.isDead) continue;

				_health.deathTimer += a_ctx.dt;
				if (_health.deathTimer < _health.releaseDelay) continue;

				// 借りているもの(ポーズ行列・ボイスなど)を Release フェーズで
				// 返してから消すため、直接消さずに解放予約を通す
				a_ctx.pWorld->AddReleaseEntity(a_pChunk->entityData[_i]);
			}
		}
	);
}
