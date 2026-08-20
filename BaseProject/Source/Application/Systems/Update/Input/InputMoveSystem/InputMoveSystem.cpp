#include "InputMoveSystem.h"

#include "Engine/ECS/World/World.h"

#include "../../../../Components/Intent/MoveIntentComponent.h"
#include "../../../../Components/Character/Robot/BoostComponent.h"

#include "Application/Components/Tag/PlayerControllTag.h"

#include "Application/Components/Character/LookAngleComponent.h"

void InputMoveSystem::Init(Engine::ECS::World& a_world)
{
	a_world.ActiveTask<const PlayerControllTag, MoveIntentComponent, LookAngleComponent,BoostComponent>(
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
			LookAngleComponent* a_playerLookArray,
			BoostComponent* a_boostArray
		)
		{
			Math::Vector3 _move = {};
			Math::Vector2 _inputMove = {};
			Math::Vector2 _look = {};

			// 移動
			_inputMove = a_ctx.pServices->pInputManager->GetAxisState("Move");

			//--------------------------------------------------------------
			// 上下の入力
			//
			// ジャンプが上、急降下(LCtrl)が下。両方押されたら打ち消し合って0になる。
			//
			// 強さは「押しているか」だけで決めたいので IsHold で取る。
			// GetButtonState が返すのは EState のビットマスク
			// (Press=1 / Hold=2 / Release=4)なので、そのまま数値として使うと
			// 押している間は2倍、離したフレームには4倍の入力が入ってしまう。
			// (押したフレームは Press|Hold なので IsHold でも拾える)
			//--------------------------------------------------------------
			const float _jumpInput = a_ctx.pServices->pInputManager->IsHold("Jump") ? 1.0f : 0.0f;
			const float _diveInput = a_ctx.pServices->pInputManager->IsHold("Dive") ? 1.0f : 0.0f;

			_move = { _inputMove.x, _jumpInput - _diveInput, _inputMove.y };

			// ブースト
			bool _isHold = a_ctx.pServices->pInputManager->IsHold("Boost");			// 押されっぱなし
			bool _isPress = a_ctx.pServices->pInputManager->IsPress("Boost");		// 押した瞬間

			// 視点
			_look = a_ctx.pServices->pInputManager->GetAxisState("Look");

			for (size_t _i = 0; _i < a_count; ++_i)
			{
				LookAngleComponent& _lookComp = a_playerLookArray[_i];
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