#include "InputMoveSystem.h"

#include "Engine/ECS/World/World.h"

#include "Application/Components/Force/VelocityComponent.h"
#include "Application/Components/Force/InertiaComponent.h"

#include "Application/Components/Tag/PlayerControllTag.h"

#include "Application/Components/Charactor/Player/PlayerLookAngleComponent.h"

void InputMoveSystem::Init(Engine::ECS::World& a_world)
{
	a_world.RegisterTask<const PlayerControllTag, VelocityComponent, PlayerLookAngleComponent>(
		Engine::ECS::ESystemType::Update,
		[]
		(
			Engine::ECS::ArchetypeChunk* a_pChunk,
			uint32_t a_count,
			float a_dt,
			const PlayerControllTag* a_tags,
			VelocityComponent* a_velocityArray,
			PlayerLookAngleComponent* a_playerLookArray
		)
		{
			DXSM::Vector3 _move = {};
			DXSM::Vector2 _inputMove = {};
			DXSM::Vector2 _look = {};

			// 移動
			_inputMove = Engine::Input::InputManager::Instance().GetAxisState("Move");
			_move = { _inputMove.x,0,_inputMove.y };

			// 視点
			_look = Engine::Input::InputManager::Instance().GetAxisState("Look");

			for (size_t _i = 0; _i < a_count; ++_i)
			{
				PlayerLookAngleComponent& _lookComp = a_playerLookArray[_i];
				_lookComp.Yaw += _look.x;

				float _rad = DirectX::XMConvertToRadians(_lookComp.Yaw);
				float _sinY = sinf(_rad);
				float _cosY = cosf(_rad);

				VelocityComponent& _velComp = a_velocityArray[_i];
				_velComp.value = {};
				_velComp.value.x += (_move.x * _cosY + _move.z * _sinY) * 5.0f;
				_velComp.value.y += _move.y;
				_velComp.value.z += (_move.z * _cosY - _move.x * _sinY) * 5.0f;
			}
		}
	);
}

void InputMoveSystem::Run(Engine::ECS::World& a_world, float a_dt)
{
	DXSM::Vector3 _move = {};

	DXSM::Vector2 _inputMove = {};
	DXSM::Vector2 _look = {};

	// 移動
	_inputMove = Engine::Input::InputManager::Instance().GetAxisState("Move");
	_move = { _inputMove.x,0,_inputMove.y };
	
	// 視点
	_look = Engine::Input::InputManager::Instance().GetAxisState("Look");

	a_world.ForEachEx<PlayerControllTag,VelocityComponent,PlayerLookAngleComponent>
		(
			[&a_world, a_dt, _move, _look]
			(
				Engine::ECS::ArchetypeChunk* a_pChunk,
				uint32_t a_count,
				PlayerControllTag* a_tags,
				VelocityComponent* a_velocityArray,
				PlayerLookAngleComponent* a_playerLookArray
				)
			{
				for (size_t _i = 0; _i < a_count; ++_i)
				{
					PlayerLookAngleComponent& _lookComp = a_playerLookArray[_i];
					_lookComp.Yaw += _look.x;

					float _rad = DirectX::XMConvertToRadians(_lookComp.Yaw);
					float _sinY = sinf(_rad);
					float _cosY = cosf(_rad);

					VelocityComponent& _velComp = a_velocityArray[_i];
					_velComp.value = {};		// 後で絶対に別の場所に移す
					_velComp.value.x += (_move.x * _cosY + _move.z * _sinY) * 5.0f;
					_velComp.value.y += _move.y;
					_velComp.value.z += (_move.z * _cosY - _move.x * _sinY) * 5.0f;
				}
			},
			Engine::ECS::Exclude<InertiaComponent>()
		);
}
