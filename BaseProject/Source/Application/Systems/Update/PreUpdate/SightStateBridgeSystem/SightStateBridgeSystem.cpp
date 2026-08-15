#include "SightStateBridgeSystem.h"

#include "Engine/ECS/World/World.h"

#include "../../../../Components/Character/TargetEntityComponent.h"
#include "../../../../Components/Resource/ActionStateComponent.h"

#include "Engine/Resource/Data/ActionStateMachineAsset/ActionStateMachineAsset.h"

//==============================================================================
// SightStateBridgeSystem
//
// SearchPlayerSystem が書いた探索結果(TargetEntityComponent)を、
// 行動 FSM(ActionStateComponent)のパラメータへ橋渡しする。
//   SeePlayer      (bool)  : 戦闘モードに入っているか(= 発見距離まで近づかれたか)
//   InAttackRange  (bool)  : 攻撃可能距離の内側か
//   TargetDistance (float) : プレイヤーまでの距離
//
// ※ パラメータ名は SeePlayer のままだが、視界コーンを廃止したので意味は
//   「視認しているか」ではなく「戦闘距離の内側か」。FSM アセット側の
//   パラメータ名を変えると既存の遷移条件が全部繋ぎ直しになるので名前は据え置く。
//
// これで 巡回→追跡→攻撃 の遷移条件を、FSM 側の設計図(パラメータ)だけで組める。
//   例) Patrol --[SeePlayer==true]--> Chase        (発見 → 追従)
//       Chase  --[InAttackRange==true]--> Aim      (攻撃圏に到達)
//
// ※ 攻撃圏の判定を FSM の TargetDistance 閾値ではなく bool にしているのは、
//   距離の設定値を敵ごと(TargetEntityComponent)に持たせるため。閾値を
//   アセットに直書きすると、そのアセットを使う敵すべてで射程が共通になる。
//
// ※ StateMachineComponent は AnimatorAsset(アニメーター)を握っているので、
//   ゲームプレイの状態機械としては ActionStateComponent を使う。
//
// ・SearchPlayerSystem と同じ PreUpdate 帯。TargetEntityComponent を
//   「書く SearchPlayerSystem → 読む本システム」の順(RAW)で自動的に後ろに回る。
// ・パラメータを読む ActionStateCommitSystem は Update 帯なので必ず後になる。
//==============================================================================
void SightStateBridgeSystem::Init(Engine::ECS::World& a_world)
{
	a_world.ActiveTask<const TargetEntityComponent, ActionStateComponent>(
		Engine::ECS::ESystemType::PreUpdate,
		"SightStateBridgeSystem",
		[](
			Engine::ECS::ArchetypeChunk*      a_pChunk,
			uint32_t                          a_count,
			const Engine::ECS::SystemContext& a_ctx,
			ActiveTag*                        a_tags,
			const TargetEntityComponent*      a_targetArray,
			ActionStateComponent*             a_stateArray
		)
		{
			// パラメータ名のハッシュは static でキャッシュ
			static const UINT s_seeHash    = StringUtility::ToHash("SeePlayer");
			static const UINT s_attackHash = StringUtility::ToHash("InAttackRange");
			static const UINT s_distHash   = StringUtility::ToHash("TargetDistance");

			auto& _pool =
				a_ctx.pWorld->GetResource<Engine::Pool::ItemPool<Engine::Resource::ActionStateInstance>>();

			for (size_t _i = 0; _i < a_count; ++_i)
			{
				const TargetEntityComponent& _target = a_targetArray[_i];
				ActionStateComponent&        _state  = a_stateArray[_i];

				auto* _pInstance = _pool.Ref(_state.instanceHandle);
				if (!_pInstance) continue;

				// 設計図(パラメータ定義を足すので Ref で可変参照を取る)
				auto* _pActionSM = a_ctx.pServices->pResourceManager->Ref(_state.actionHandle);
				if (!_pActionSM) continue;

				_pActionSM->SetBoolParam(*_pInstance, s_seeHash, "SeePlayer", _target.isFind);
				_pActionSM->SetBoolParam(*_pInstance, s_attackHash, "InAttackRange", _target.isInAttackRange);
				_pActionSM->SetFloatParam(*_pInstance, s_distHash, "TargetDistance", _target.distance);
			}
		}
	);
}
