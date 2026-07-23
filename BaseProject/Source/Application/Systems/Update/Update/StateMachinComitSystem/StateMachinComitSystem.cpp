#include "StateMachinComitSystem.h"
#include "Engine/ECS/World/World.h"

#include "../../../../Components/Resource/StateMachineComponent.h"

#include "../../../../../Engine/Resource/Manager/ResourceManager/ResourceManager.h"

void StateMachinComitSystem::Init(Engine::ECS::World& a_world)
{
	a_world.ActiveTask<StateMachineComponent>(
		Engine::ECS::ESystemType::Update,
		"StateMachinComitSystem",
		[]
		(
			Engine::ECS::ArchetypeChunk* a_pChunk,
			uint32_t a_count,
			const Engine::ECS::SystemContext& a_ctx,
			ActiveTag* a_tags,
			StateMachineComponent* a_smArray
			)
		{
			for (size_t _i = 0; _i < a_count; ++_i)
			{
				StateMachineComponent& _smComp = a_smArray[_i];

				// ステートマシン取得
				const auto* _pStateMacihne = a_ctx.pServices->pResourceManager->Get(_smComp.stateMachineHandle);

				// 入力されたステートマシンの値を使って、現在のステートを更新
				// インスタンスの実体を取得
				auto& _stateInstancePool = a_ctx.pWorld->GetResource<Engine::Pool::ItemPool<Engine::Resource::StateMachinInstance>>();
				auto* _pInstanceData = _stateInstancePool.Ref(_smComp.instanceHandle);
				if (!_pInstanceData) continue;
				

				// 読み込みチェック
				if (!_pStateMacihne || !_pInstanceData) continue;

				// 初回起動時のセットアップ
				if (_smComp.currentStateHash == 0)
				{
					_smComp.currentStateHash = _pStateMacihne->GetDefaultStartHash();
					_smComp.prevStateHash = _smComp.currentStateHash;
					_smComp.currentTime = 0.0f;
				}

				// 現在ステートの経過時間
				_smComp.currentTime += a_ctx.dt;

				// 遷移の評価
				UINT _nextStateHash = _pStateMacihne->EvaluateNextState(_smComp.currentStateHash, *_pInstanceData);

				// 遷移が発生したときの処理 
				if (_nextStateHash != _smComp.currentStateHash)
				{
					_smComp.prevStateHash = _smComp.currentStateHash;
					_smComp.currentStateHash = _nextStateHash;
					_smComp.currentTime = 0.0f; // 遷移したら時間をリセット
				}
			}
		}
	);
}
