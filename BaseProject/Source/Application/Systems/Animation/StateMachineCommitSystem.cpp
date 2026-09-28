#include "StateMachineCommitSystem.h"
#include "Application/ECS/World/APPWorld.h"

#include "Application/Components/Animation/AnimatorComponent.h"

#include "Engine/Resource/Manager/ResourceManager/ResourceManager.h"

void StateMachineCommitSystem::Init(App::ECS::APPWorld& a_world)
{
	// 自分のチャンクのステートだけを書く(定義とインスタンスは読むだけ)ので、ワーカーで回す
	a_world.ActiveJobTask<AnimatorComponent>(
		Engine::ECS::ESystemType::Update,
		"StateMachineCommitSystem",
		[]
		(
			Engine::ECS::Chunk* a_pChunk,
			uint32_t a_count,
			const Engine::ECS::SystemContext& a_ctx,
			ActiveTag* a_tags,
			AnimatorComponent* a_animatorArray
			)
		{
			for (size_t _i = 0; _i < a_count; ++_i)
			{
				AnimatorLayer& _layer = a_animatorArray[_i].baseLayer;

				// ステートマシン取得
				const auto* _pStateMacihne = a_ctx.pServices->pResourceManager->Get(_layer.animatorHandle);

				// 入力されたステートマシンの値を使って、現在のステートを更新
				// インスタンスの実体を取得
				auto& _stateInstancePool = a_ctx.pWorld->GetResource<Engine::Pool::ItemPool<Engine::Resource::StateMachineInstance>>();
				auto* _pInstanceData = _stateInstancePool.Ref(_layer.instanceHandle);
				if (!_pInstanceData) continue;
				

				// 読み込みチェック
				if (!_pStateMacihne || !_pInstanceData) continue;

				// 初回起動時のセットアップ
				if (_layer.currentStateHash == 0)
				{
					_layer.currentStateHash = _pStateMacihne->GetDefaultStartHash();
					_layer.prevStateHash = _layer.currentStateHash;
					_layer.stateTime = 0.0f;
					_layer.clipTime = 0.0f;
				}

				// 現在ステートの経過時間
				_layer.stateTime += a_ctx.dt;

				// 遷移の評価
				UINT _nextStateHash = _pStateMacihne->EvaluateNextState(_layer.currentStateHash, *_pInstanceData);

				// 遷移が発生したときの処理
				if (_nextStateHash != _layer.currentStateHash)
				{
					_layer.prevStateHash = _layer.currentStateHash;
					_layer.currentStateHash = _nextStateHash;

					// 遷移したら時間をリセットする。
					// クリップの再生位置も戻す(以前は戻しておらず、次のクリップが
					// 前のクリップの再生位置の途中から始まっていた)
					_layer.stateTime = 0.0f;
					_layer.clipTime = 0.0f;
				}
			}
		}
	)
	// 遷移の条件に使うインスタンスの値を読む
	.ReadsResource<Engine::Pool::ItemPool<Engine::Resource::StateMachineInstance>>();
}
