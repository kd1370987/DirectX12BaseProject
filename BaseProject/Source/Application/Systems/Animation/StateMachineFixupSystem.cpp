#include "StateMachineFixupSystem.h"

#include "Application/ECS/World/APPWorld.h"

#include "Application/Components/Core/PhaseTag/PostDeserializeTag.h"
#include "Application/Components/Animation/AnimatorComponent.h"

#include "Engine/Resource/Data/AnimatorAsset/AnimatorAsset.h"

void StateMachineFixupSystem::Init(App::ECS::APPWorld& a_world)
{
	a_world.PostDeserializeTask<AnimatorComponent>(
		Engine::ECS::ESystemType::PostDeserialize,
		"StateMachineFixupSystem",
		[]
		(
			Engine::ECS::Chunk* a_pChunk,
			uint32_t a_count,
			const Engine::ECS::SystemContext& a_ctx,
			PostDeserializeTag* a_tag,
			AnimatorComponent* a_animatorArray
		)
		{
			for (size_t _i = 0; _i < a_count; ++_i)
			{
				AnimatorLayer& _layer = a_animatorArray[_i].baseLayer;

				// 入り直し(設計図の差し替えなど)に備えて、ステートは最初からやり直す。
				// 前の設計図のステートのハッシュが残っていると、新しい設計図では引けない
				_layer.prevStateHash = 0;
				_layer.currentStateHash = 0;
				_layer.stateTime = 0.0f;
				_layer.clipTime = 0.0f;

				// 設計図をGUIDから取得してロードした結果のハンドルを取得
				if (_layer.animatorGUID != Engine::DefaultGUID)
				{
					// 設計図ロード
					a_ctx.pServices->pResourceManager->AcquireImmediate(
						_layer.animatorHandle, _layer.animatorGUID);

					// パラメータの実体。
					// 入り直しで前の実体がまだ残っていれば(Release フェーズを通らなかった)、
					// 作り直さずに中身だけ初期化して使い回す
					auto& _stateInstancePool =
						a_ctx.pWorld->GetResource<Engine::Pool::ItemPool<Engine::Resource::StateMachineInstance>>();
					if (auto* _pInstance = _stateInstancePool.Ref(_layer.instanceHandle))
					{
						*_pInstance = {};
					}
					else
					{
						Engine::Resource::StateMachineInstance _instance = {};
						_layer.instanceHandle = _stateInstancePool.Add(std::move(_instance));
					}
				}
			}
		}
	);
}
