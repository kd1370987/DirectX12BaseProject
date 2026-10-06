#include "StateMachineCommitSystem.h"
#include "Application/ECS/World/APPWorld.h"

#include "Application/Components/Animation/AnimatorComponent.h"
#include "Application/Components/Animation/UpperAnimatorComponent.h"

#include "Engine/Resource/Manager/ResourceManager/ResourceManager.h"

namespace
{
	// レイヤー1枚ぶんのステートの更新 : 遷移を評価し、遷移したら時間を戻す
	void CommitLayer(const Engine::ECS::SystemContext& a_ctx, AnimatorLayer& a_layer)
	{
		// ステートマシン取得
		const auto* _pStateMacihne = a_ctx.pServices->pResourceManager->Get(a_layer.animatorHandle);

		// 入力されたステートマシンの値を使って、現在のステートを更新
		// インスタンスの実体を取得
		auto& _stateInstancePool = a_ctx.pWorld->RefResource<Engine::Pool::ItemPool<Engine::Resource::StateMachineInstance>>();
		auto* _pInstanceData = _stateInstancePool.Ref(a_layer.instanceHandle);
		if (!_pInstanceData) return;
		

		// 読み込みチェック
		if (!_pStateMacihne || !_pInstanceData) return;

		// 初回起動時のセットアップ
		if (a_layer.currentStateHash == 0)
		{
			a_layer.currentStateHash = _pStateMacihne->GetDefaultStartHash();
			a_layer.prevStateHash = a_layer.currentStateHash;
			a_layer.stateTime = 0.0f;
			a_layer.clipTime = 0.0f;
		}

		// 現在ステートの経過時間
		a_layer.stateTime += a_ctx.dt;

		// 遷移の評価
		UINT _nextStateHash = _pStateMacihne->EvaluateNextState(a_layer.currentStateHash, *_pInstanceData);

		// 遷移が発生したときの処理
		if (_nextStateHash != a_layer.currentStateHash)
		{
			a_layer.prevStateHash = a_layer.currentStateHash;
			a_layer.currentStateHash = _nextStateHash;

			// 遷移したら時間をリセットする。
			// クリップの再生位置も戻す(以前は戻しておらず、次のクリップが
			// 前のクリップの再生位置の途中から始まっていた)
			a_layer.stateTime = 0.0f;
			a_layer.clipTime = 0.0f;
		}
	}
}

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
				CommitLayer(a_ctx, a_animatorArray[_i].baseLayer);
			}
		}
	)
	// 遷移の条件に使うインスタンスの値を読む
	.ReadsResource<Engine::Pool::ItemPool<Engine::Resource::StateMachineInstance>>();

	// 上に重ねるレイヤー
	a_world.ActiveJobTask<UpperAnimatorComponent>(
		Engine::ECS::ESystemType::Update,
		"StateMachineCommitSystem_Upper",
		[]
		(
			Engine::ECS::Chunk* a_pChunk,
			uint32_t a_count,
			const Engine::ECS::SystemContext& a_ctx,
			ActiveTag* a_tags,
			UpperAnimatorComponent* a_animatorArray
			)
		{
			for (size_t _i = 0; _i < a_count; ++_i)
			{
				CommitLayer(a_ctx, a_animatorArray[_i].layer);
			}
		}
	)
	// 遷移の条件に使うインスタンスの値を読む
	.ReadsResource<Engine::Pool::ItemPool<Engine::Resource::StateMachineInstance>>();
}
