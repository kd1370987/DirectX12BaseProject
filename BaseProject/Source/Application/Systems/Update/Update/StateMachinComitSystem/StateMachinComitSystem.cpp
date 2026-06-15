#include "StateMachinComitSystem.h"
#include "Engine/ECS/World/World.h"

#include "../../../../Components/Resource/StateMachineComponent.h"

#include "../../../../../Engine/Resource/Manager/ResourceManager/ResourceManager.h"
#include "../../../../../Engine/Resource/Manager/InstancePoolManager/InstancePoolManager.h"

void StateMachinComitSystem::Init(Engine::ECS::World& a_world)
{
	a_world.ActiveTask<StateMachineComponent>(
		Engine::ECS::ESystemType::Update,
		"StateMachinComitSystem",
		[]
		(
			Engine::ECS::ArchetypeChunk* a_pChunk,
			uint32_t a_count,
			float a_dt,
			ActiveTag* a_tags,
			StateMachineComponent* a_smArray
			)
		{
			for (size_t _i = 0; _i < a_count; ++_i)
			{
				StateMachineComponent& _smComp = a_smArray[_i];

				// 入力されたステートマシンの値を使って、現在のステートを更新
				const auto* _pStateMacihne = Engine::Resource::ResourceManager::Instance().Get(_smComp.stateMachineHandle);
				auto* _pInstanceData = Engine::Resource::InstancePoolManager::Instance().Ref(_smComp.instanceHandle);

				// 読み込みチェック
				if (!_pStateMacihne || _pInstanceData) continue;

				// 初回起動時のセットアップ
				if (_smComp.currentStateHash == 0)
				{
					_smComp.currentStateHash = _pStateMacihne->GetDefaultStartHash();
					_smComp.prevStateHash = _smComp.currentStateHash;
					_smComp.currentTime = 0.0f;
				}

				// 現在ステートの経過時間
				_smComp.currentTime += a_dt;

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
