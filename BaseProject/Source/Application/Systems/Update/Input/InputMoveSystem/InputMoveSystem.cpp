#include "InputMoveSystem.h"

#include "Engine/ECS/World/World.h"

#include "../../../../Components/Intent/MoveIntentComponent.h"
#include "../../../../Components/Charactor/Robot/BoostComponent.h"
#include "Application/Components/Force/InertiaComponent.h"

#include "Application/Components/Tag/PlayerControllTag.h"

#include "Application/Components/Charactor/Player/PlayerLookAngleComponent.h"

void InputMoveSystem::Init(Engine::ECS::World& a_world)
{
	a_world.ActiveTask<const PlayerControllTag, MoveIntentComponent, PlayerLookAngleComponent,BoostComponent>(
		Engine::ECS::ESystemType::Input,
		"InputMoveSystem",
		[]
		(
			Engine::ECS::ArchetypeChunk* a_pChunk,
			uint32_t a_count,
			const Engine::ECS::SystemContext& a_ctx,
			ActiveTag* a_ActiveTag,
			const PlayerControllTag* a_tags,
			MoveIntentComponent* a_moveIntentArray,
			PlayerLookAngleComponent* a_playerLookArray,
			BoostComponent* a_boostArray
		)
		{
			DXSM::Vector3 _move = {};
			DXSM::Vector2 _inputMove = {};
			DXSM::Vector2 _look = {};

			// 移動
			_inputMove = Engine::Input::InputManager::Instance().GetAxisState("Move");
			float _jumpInput = Engine::Input::InputManager::Instance().GetButtonState("Jump");
			_move = { _inputMove.x,_jumpInput,_inputMove.y };

			// ブースト
			bool _isHold = Engine::Input::InputManager::Instance().IsHold("Boost");			// 押されっぱなし
			bool _isPress = Engine::Input::InputManager::Instance().IsPress("Boost");		// 押した瞬間

			// 視点
			_look = Engine::Input::InputManager::Instance().GetAxisState("Look");

			for (size_t _i = 0; _i < a_count; ++_i)
			{
				PlayerLookAngleComponent& _lookComp = a_playerLookArray[_i];
				MoveIntentComponent& _intentComp = a_moveIntentArray[_i];
				BoostComponent& _boostComp = a_boostArray[_i];

				_lookComp.Yaw += _look.x;
				_lookComp.Pitch += _look.y;
				_lookComp.Pitch = std::clamp(_lookComp.Pitch, -_lookComp.maxPitch, _lookComp.maxPitch);

				_intentComp.value = {};
				_intentComp.value = _move;

				_boostComp.isBoostTriger = _isPress;
				_boostComp.isBoostIntent = _isHold;
			}
		}
	);
}