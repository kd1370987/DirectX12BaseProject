#include "ActionStateCommitSystem.h"

#include "Engine/ECS/World/World.h"

#include "Application/Components/Resource/ActionStateComponent.h"

#include "Engine/Resource/Data/ActionStateMachineAsset/ActionStateMachineAsset.h"

void ActionStateCommitSystem::Init(Engine::ECS::World& a_world)
{
	a_world.ActiveTask<ActionStateComponent>(
		Engine::ECS::ESystemType::Update,
		"ActionStateCommitSystem",
		[]
		(
			Engine::ECS::ArchetypeChunk* a_pChunk,
			uint32_t a_count,
			const Engine::ECS::SystemContext& a_ctx,
			ActiveTag* a_tags,
			ActionStateComponent* a_array
			)
		{
			for (size_t _i = 0; _i < a_count; ++_i)
			{
				ActionStateComponent& _comp = a_array[_i];

				const auto* _pSM =
					Engine::Resource::ResourceManager::Instance().Get(_comp.actionHandle);

				auto& _pool =
					a_ctx.pWorld->GetResource<Engine::Pool::ItemPool<Engine::Resource::ActionStateInstance>>();
				auto* _pInstance = _pool.Ref(_comp.instanceHandle);

				if (!_pSM || !_pInstance) continue;

				// 初回セットアップ
				if (_comp.currentStateHash == 0)
				{
					_comp.currentStateHash = _pSM->GetDefaultStartHash();
					_comp.prevStateHash = _comp.currentStateHash;
					_comp.currentTime = 0.0f;
				}

				_comp.currentTime += a_ctx.dt;

				// 遷移評価
				UINT _next = _pSM->EvaluateNextState(_comp.currentStateHash, *_pInstance);
				if (_next != _comp.currentStateHash)
				{
					_comp.prevStateHash = _comp.currentStateHash;
					_comp.currentStateHash = _next;
					_comp.currentTime = 0.0f;
				}
			}
		}
	);
}
