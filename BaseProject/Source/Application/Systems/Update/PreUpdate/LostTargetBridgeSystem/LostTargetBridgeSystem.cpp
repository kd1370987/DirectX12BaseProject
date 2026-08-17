#include "LostTargetBridgeSystem.h"

#include "Engine/ECS/World/World.h"

#include "../../../../Components/Character/PatrolComponent.h"
#include "../../../../Components/Resource/ActionStateComponent.h"

#include "Engine/Resource/Data/ActionStateMachineAsset/ActionStateMachineAsset.h"

//==============================================================================
// LostTargetBridgeSystem
//
// EnemyMoveIntentSystem が進める「見失い探索」のフェーズ(PatrolComponent.lostPhase)を、
// 行動 FSM(ActionStateComponent)のパラメータへ橋渡しする。
//   LostSearch (bool) : 見失った地点を探索中か(移動中 or 見渡し中)
//
// これで LostTarget ステートの出入りを FSM の設計図だけで組める。
//   Chase / Attack --[SeePlayer == false]--> LostTarget
//   LostTarget     --[SeePlayer == true ]--> Chase        (再発見)
//   LostTarget     --[LostSearch == false]--> Patrol      (探索終了)
//
// ・SeePlayer を書く SightStateBridgeSystem とは別システムにしてある。
//   こちらは PatrolComponent が必要なので、まとめてしまうと
//   PatrolComponent を持たない敵から SeePlayer まで消えてしまう。
// ・PatrolComponent を持たない敵では LostSearch が書かれず、定義のデフォルト
//   (false)で評価される。つまり LostTarget に入っても即 Patrol へ抜ける
//   =従来どおりの挙動になる。
// ・PatrolComponent を「読む」ので、それを書く EnemyMoveIntentSystem の後ろへ
//   自動的に回る(同じ PreUpdate 帯)。
//==============================================================================
void LostTargetBridgeSystem::Init(Engine::ECS::World& a_world)
{
	a_world.ActiveTask<const PatrolComponent, ActionStateComponent>(
		Engine::ECS::ESystemType::PreUpdate,
		"LostTargetBridgeSystem",
		[](
			Engine::ECS::ArchetypeChunk*      a_pChunk,
			uint32_t                          a_count,
			const Engine::ECS::SystemContext& a_ctx,
			ActiveTag*                        a_tags,
			const PatrolComponent*            a_patrolArray,
			ActionStateComponent*             a_stateArray
		)
		{
			// パラメータ名のハッシュは static でキャッシュ
			static const UINT s_lostHash = Engine::String::ToHash("LostSearch");

			auto& _pool =
				a_ctx.pWorld->GetResource<Engine::Pool::ItemPool<Engine::Resource::ActionStateInstance>>();

			for (size_t _i = 0; _i < a_count; ++_i)
			{
				const PatrolComponent& _patrol = a_patrolArray[_i];
				ActionStateComponent&  _state  = a_stateArray[_i];

				auto* _pInstance = _pool.Ref(_state.instanceHandle);
				if (!_pInstance) continue;

				// 設計図(パラメータ定義を足すので Ref で可変参照を取る)
				auto* _pActionSM = a_ctx.pServices->pResourceManager->Ref(_state.actionHandle);
				if (!_pActionSM) continue;

				const bool _isLostSearch = (_patrol.lostPhase != ELostPhase::None);
				_pActionSM->SetBoolParam(*_pInstance, s_lostHash, "LostSearch", _isLostSearch);
			}
		}
	);
}
