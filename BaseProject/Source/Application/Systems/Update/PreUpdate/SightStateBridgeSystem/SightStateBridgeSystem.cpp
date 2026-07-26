#include "SightStateBridgeSystem.h"

#include "Engine/ECS/World/World.h"

#include "../../../../Components/Character/TargetEntityComponent.h"
#include "../../../../Components/Resource/StateMachineComponent.h"

//==============================================================================
// SightStateBridgeSystem
//
// SearchPlayerSystem が書いた探索結果(TargetEntityComponent)を、
// ステートマシンのパラメータへ橋渡しする。
//   SeePlayer      (bool)  : プレイヤーを視認しているか
//   TargetDistance (float) : プレイヤーまでの距離
//
// これで巡回→追跡→攻撃 の遷移条件を、ステートマシン側の設計図(パラメータ)だけで
// 組めるようになる。
//
// ・SearchPlayerSystem と同じ PreUpdate 帯に置く。
//   TargetEntityComponent を「書く SearchPlayerSystem → 読む本システム」の順(RAW)
//   になるので、依存解決で自動的に後ろに回る(登録も後ろにしておく)。
// ・パラメータを読む StateMachineCommitSystem は Update 帯なので、
//   フェーズ順で必ず本システムより後になる。
//==============================================================================
void SightStateBridgeSystem::Init(Engine::ECS::World& a_world)
{
	a_world.ActiveTask<const TargetEntityComponent, StateMachineComponent>(
		Engine::ECS::ESystemType::PreUpdate,
		"SightStateBridgeSystem",
		[](
			Engine::ECS::ArchetypeChunk*      a_pChunk,
			uint32_t                          a_count,
			const Engine::ECS::SystemContext& a_ctx,
			ActiveTag*                        a_tags,
			const TargetEntityComponent*      a_targetArray,
			StateMachineComponent*            a_smArray
		)
		{
			// パラメータ名のハッシュは static でキャッシュ
			static const UINT s_seeHash  = StringUtility::ToHash("SeePlayer");
			static const UINT s_distHash = StringUtility::ToHash("TargetDistance");

			auto& _pool =
				a_ctx.pWorld->GetResource<Engine::Pool::ItemPool<Engine::Resource::StateMachineInstance>>();

			for (size_t _i = 0; _i < a_count; ++_i)
			{
				const TargetEntityComponent& _target = a_targetArray[_i];
				StateMachineComponent&       _smComp = a_smArray[_i];

				auto* _pInstance = _pool.Ref(_smComp.instanceHandle);
				if (!_pInstance) continue;

				_pInstance->boolParams[s_seeHash]   = _target.isFind;
				_pInstance->floatParams[s_distHash] = _target.distance;
			}
		}
	);
}
