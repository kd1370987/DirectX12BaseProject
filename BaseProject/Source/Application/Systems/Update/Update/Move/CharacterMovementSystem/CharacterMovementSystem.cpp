#include "CharacterMovementSystem.h"

#include "Engine/ECS/World/World.h"
#include "Engine/Resource/Manager/ResourceManager/ResourceManager.h"

#include "../../../../../Components/Intent/MoveIntentComponent.h"
#include "../../../../../Components/Force/VelocityComponent.h"
#include "../../../../../Components/Character/Player/PlayerLookAngleComponent.h"

#include "../../../../../Components/Resource/StateMachineComponent.h"

void CharacterMovementSystem::Init(Engine::ECS::World& a_world)
{
	a_world.ActiveTask<const PlayerLookAngleComponent, const MoveIntentComponent,VelocityComponent,StateMachineComponent>(
		Engine::ECS::ESystemType::Update,
		"CharacterMovementSystem",
		[]
		(
			Engine::ECS::ArchetypeChunk* a_pChunk,
			uint32_t a_count,
			const Engine::ECS::SystemContext& a_ctx,
			ActiveTag* a_tags,
			const PlayerLookAngleComponent* a_lookArray,
			const MoveIntentComponent* a_intentArray,
			VelocityComponent* a_velArray,
			StateMachineComponent* a_stateMachineArray
			)
		{
			for (size_t _i = 0; _i < a_count; ++_i)
			{
				const PlayerLookAngleComponent& _lookComp = a_lookArray[_i];
				const MoveIntentComponent& _moveIntent = a_intentArray[_i];
				VelocityComponent& _velComp = a_velArray[_i];
				StateMachineComponent& _stateMachineComp = a_stateMachineArray[_i];

				float _rad = DirectX::XMConvertToRadians(_lookComp.Yaw);
				float _sinY = sinf(_rad);
				float _cosY = cosf(_rad);

				_velComp.value.x = (_moveIntent.value.x * _cosY + _moveIntent.value.z * _sinY) * 5.0f;
				_velComp.value.y = _moveIntent.value.y * _moveIntent.jumpPow;
				_velComp.value.z = (_moveIntent.value.z * _cosY - _moveIntent.value.x * _sinY) * 5.0f;
			}
		}
	);
}
