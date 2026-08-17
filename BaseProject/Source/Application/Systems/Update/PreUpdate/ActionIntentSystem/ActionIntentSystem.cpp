#include "ActionIntentSystem.h"

#include "Engine/ECS/World/World.h"

#include "Application/Components/Intent/MoveIntentComponent.h"
#include "Application/Components/Intent/ActionIntentComponent.h"
#include "Application/Components/Resource/ActionStateComponent.h"

#include "Engine/Resource/Data/ActionStateMachineAsset/ActionStateMachineAsset.h"

void ActionIntentSystem::Init(Engine::ECS::World& a_world)
{
	a_world.ActiveTask<const ActionIntentComponent,const MoveIntentComponent, ActionStateComponent>(
		Engine::ECS::ESystemType::PreUpdate,
		"ActionIntentSystem",
		[]
		(
			Engine::ECS::ArchetypeChunk* a_pChunk,
			uint32_t a_count,
			const Engine::ECS::SystemContext& a_ctx,
			ActiveTag* a_tags,
			const ActionIntentComponent* a_actionIntentArray,
			const MoveIntentComponent* a_moveIntentArray,
			ActionStateComponent* a_array
			)
		{
			// パラメータ名ハッシュはstaticで保持
			static const UINT s_speedHash = Engine::String::ToHash("Speed");
			static const UINT s_shoot = Engine::String::ToHash("Shoot");
			static const UINT s_aim = Engine::String::ToHash("Aim");

			for (size_t _i = 0; _i < a_count; ++_i)
			{
				const ActionIntentComponent& _actionIntent = a_actionIntentArray[_i];
				const MoveIntentComponent& _intent = a_moveIntentArray[_i];
				ActionStateComponent& _comp = a_array[_i];

				auto& _pool =
					a_ctx.pWorld->GetResource<Engine::Pool::ItemPool<Engine::Resource::ActionStateInstance>>();
				auto* _pInstance = _pool.Ref(_comp.instanceHandle);
				if (!_pInstance) continue;

				// 設計図(パラメータ定義を足すので Ref で可変参照を取る)
				auto* _pActionSM = a_ctx.pServices->pResourceManager->Ref(_comp.actionHandle);
				if (!_pActionSM) continue;

				// 移動入力の大きさを Speed パラメータへ
				float _speed = std::sqrt(
					(_intent.value.x * _intent.value.x) +
					(_intent.value.z * _intent.value.z));
				_pActionSM->SetFloatParam(*_pInstance, s_speedHash, "Speed", _speed);

				// 銃関係
				_pActionSM->SetBoolParam(*_pInstance, s_shoot, "Shoot", _actionIntent.isGunShoot);
				_pActionSM->SetBoolParam(*_pInstance, s_aim, "Aim", _actionIntent.isAiming);
			}
		}
	);
}
