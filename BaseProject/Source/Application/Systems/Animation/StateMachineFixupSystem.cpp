#include "StateMachineFixupSystem.h"

#include "Application/ECS/World/APPWorld.h"

#include "Application/Components/Core/PhaseTag/PostDeserializeTag.h"
#include "Application/Components/Animation/AnimatorComponent.h"
#include "Application/Components/Animation/UpperAnimatorComponent.h"

#include "Engine/Resource/Data/AnimatorAsset/AnimatorAsset.h"

namespace App::System
{
	namespace
	{
		// レイヤー1枚ぶんの入り直し : ステートを最初からにし、設計図とパラメータの実体を取り直す
		void FixupLayer(const Engine::ECS::SystemContext& a_ctx, Component::AnimatorLayer& a_layer)
		{
			// 入り直し(設計図の差し替えなど)に備えて、ステートは最初からやり直す。
			// 前の設計図のステートのハッシュが残っていると、新しい設計図では引けない
			a_layer.prevStateHash = 0;
			a_layer.currentStateHash = 0;
			a_layer.stateTime = 0.0f;
			a_layer.clipTime = 0.0f;

			// 設計図をGUIDから取得してロードした結果のハンドルを取得
			if (a_layer.animatorGUID != Core::DEFAULT_GUID)
			{
				// 設計図ロード
				a_ctx.pServices->pResourceManager->AcquireImmediate(
					a_layer.animatorHandle, a_layer.animatorGUID);

				// パラメータの実体。
				// 入り直しで前の実体がまだ残っていれば(Release フェーズを通らなかった)、
				// 作り直さずに中身だけ初期化して使い回す
				auto& _stateInstancePool =
					a_ctx.pWorld->RefResource<Engine::Pool::ItemPool<Engine::Resource::StateMachineInstance>>();
				if (auto* _pInstance = _stateInstancePool.Ref(a_layer.instanceHandle))
				{
					*_pInstance = {};
				}
				else
				{
					Engine::Resource::StateMachineInstance _instance = {};
					a_layer.instanceHandle = _stateInstancePool.Add(std::move(_instance));
				}
			}
		}
	}

	void StateMachineFixupSystem::Init(App::ECS::APPWorld& a_world)
	{
		a_world.PostDeserializeTask<Component::AnimatorComponent>(
			Engine::ECS::ESystemType::PostDeserialize,
			"StateMachineFixupSystem",
			[]
			(
				Engine::ECS::Chunk* /*a_pChunk*/,
				uint32_t a_count,
				const Engine::ECS::SystemContext& a_ctx,
				Component::PostDeserializeTag* /*a_tag*/,
				Component::AnimatorComponent* a_animatorArray
			)
			{
				for (size_t _i = 0; _i < a_count; ++_i)
				{
					FixupLayer(a_ctx, a_animatorArray[_i].baseLayer);
				}
			}
		);

		// 上に重ねるレイヤー
		a_world.PostDeserializeTask<Component::UpperAnimatorComponent>(
			Engine::ECS::ESystemType::PostDeserialize,
			"StateMachineFixupSystem_Upper",
			[]
			(
				Engine::ECS::Chunk* /*a_pChunk*/,
				uint32_t a_count,
				const Engine::ECS::SystemContext& a_ctx,
				Component::PostDeserializeTag* /*a_tag*/,
				Component::UpperAnimatorComponent* a_animatorArray
			)
			{
				for (size_t _i = 0; _i < a_count; ++_i)
				{
					FixupLayer(a_ctx, a_animatorArray[_i].layer);
				}
			}
		);
	}
}
