#include "GunShootSystem.h"

#include "Engine/ECS/World/World.h"

#include "Application/Components/Resource/ActionStateComponent.h"
#include "Application/Components/Force/VelocityComponent.h"
#include "../../../../Components/Intent/ActionIntentComponent.h"

#include "Engine/Resource/Data/ActionStateMachineAsset/ActionStateMachineAsset.h"

// 玉用コンポーネント
#include "../../../../Components/Transform/LocalTransformComponent.h"
#include "../../../../Components/Transform/WorldMatrixComponent.h"
#include "../../../../Components/Resource/ModelComponent.h"

void GunShootSystem::Init(Engine::ECS::World& a_world)
{
	a_world.ActiveTask<const ActionStateComponent, const ActionIntentComponent,VelocityComponent>(
		Engine::ECS::ESystemType::Update,
		"ActionBehaviorSystem",
		[&a_world]
		(
			Engine::ECS::ArchetypeChunk* a_pChunk,
			uint32_t a_count,
			float a_dt,
			ActiveTag* a_tags,
			const ActionStateComponent* a_stateArray,
			const ActionIntentComponent* a_actionIntentArray,
			VelocityComponent* a_velArray
			)
		{
			for (size_t _i = 0; _i < a_count; ++_i)
			{
				const ActionStateComponent& _state = a_stateArray[_i];
				const ActionIntentComponent& _intent = a_actionIntentArray[_i];
				VelocityComponent& _vel = a_velArray[_i];

				const auto* _pSM =
					Engine::Resource::ResourceManager::Instance().Get(_state.actionHandle);
				if (!_pSM) continue;

				const auto* _node = _pSM->GetStateNode(_state.currentStateHash);
				if (!_node) continue;

				// このステート中に移動できないなら水平速度を止める。
				// 移動できるなら状態ごとの速度倍率を掛ける(=状態が行動を決める)。
				if (_intent.isGunShoot)
				{
					ENGINE_LOG("Shoot");
					Engine::ECS::Signature _sig = {};
					_sig.set(a_world.GetCompTypeID<LocalTransformComponent>());
					_sig.set(a_world.GetCompTypeID<WorldMatrixComponent>());
					//_sig.set(a_world.GetCompTypeID<WorldMatrixComponent>());
					a_world.AddEntity(_sig);
				}
			}
		}
	);
}
