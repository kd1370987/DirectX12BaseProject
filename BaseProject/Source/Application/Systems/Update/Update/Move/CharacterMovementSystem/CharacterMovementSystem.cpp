#include "CharacterMovementSystem.h"

#include "Application/ECS/World/APPWorld.h"
#include "Engine/Resource/Manager/ResourceManager/ResourceManager.h"

#include "../../../../../Components/Intent/MoveIntentComponent.h"
#include "../../../../../Components/Force/VelocityComponent.h"
#include "../../../../../Components/Force/MovementComponent.h"
#include "../../../../../Components/Character/LookAngleComponent.h"

#include "../../../../../Components/Resource/StateMachineComponent.h"

//==============================================================================
// CharacterMovementSystem
//
// プレイヤーの移動入力(MoveIntent ＝ カメラ相対)を目標速度へ変換する。
//
// ・移動速度は MovementComponent.moveSpeed。加速度/減速度で実速度へ均すのは
//   MovementIntegrationSystem(Physics 帯)の担当なので、ここは目標値を作るだけ。
// ・StateMachineComponent は中身を使わず、対象の絞り込みにだけ使っている
//   (群れのリーダーのように、視点角と移動入力を持つがステートマシンを持たないものを外すため)。
//   書かないので const。書き込み扱いにすると、使ってもいないのに依存の辺が張られて循環の元になる
//==============================================================================
void CharacterMovementSystem::Init(App::ECS::APPWorld& a_world)
{
	a_world.ActiveTask<const LookAngleComponent, const MoveIntentComponent, const MovementComponent,
		VelocityComponent, const StateMachineComponent>(
		Engine::ECS::ESystemType::Update,
		"CharacterMovementSystem",
		[]
		(
			Engine::ECS::Chunk* a_pChunk,
			uint32_t a_count,
			const Engine::ECS::SystemContext& a_ctx,
			ActiveTag* a_tags,
			const LookAngleComponent* a_lookArray,
			const MoveIntentComponent* a_intentArray,
			const MovementComponent* a_movementArray,
			VelocityComponent* a_velArray,
			const StateMachineComponent*		// 絞り込みにだけ使う
			)
		{
			for (size_t _i = 0; _i < a_count; ++_i)
			{
				const LookAngleComponent& _lookComp = a_lookArray[_i];
				const MoveIntentComponent& _moveIntent = a_intentArray[_i];
				const MovementComponent& _moveComp = a_movementArray[_i];
				VelocityComponent& _velComp = a_velArray[_i];

				float _rad = DirectX::XMConvertToRadians(_lookComp.Yaw);
				float _sinY = sinf(_rad);
				float _cosY = cosf(_rad);

				const float _speed = _moveComp.moveSpeed;

				_velComp.value.x = (_moveIntent.value.x * _cosY + _moveIntent.value.z * _sinY) * _speed;
				_velComp.value.y = _moveIntent.value.y * _moveIntent.jumpPow;
				_velComp.value.z = (_moveIntent.value.z * _cosY - _moveIntent.value.x * _sinY) * _speed;
			}
		}
	);
}
