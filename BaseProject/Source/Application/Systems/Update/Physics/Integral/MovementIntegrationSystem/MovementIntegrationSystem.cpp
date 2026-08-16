#include "MovementIntegrationSystem.h"

#include "Engine/ECS/World/World.h"

#include "Application/Components/Force/VelocityComponent.h"
#include "Application/Components/Force/MovementComponent.h"
#include "Application/Components/Transform/LocalTransformComponent.h"

//==============================================================================
// MovementIntegrationSystem
//
// VelocityComponent(目標速度)へ MovementComponent の加速度/減速度で追従させ、
// その実速度(MovementComponent.velocity)で座標を進める。
// 旧 InertiaIntegrationSystem(時定数による指数追従)の置き換え。
//
// ・加減速をかけるのは水平(XZ)だけ。上下は重力/ジャンプ/ブーストが直接作る値なので
//   そのまま通す。加減速を挟むと落下や着地が鈍るため。
// ・加速か減速かは「目標速度が今の速さより速いか」で選ぶ。向きを変えるだけで
//   速さが変わらない旋回中は加速度側になる。
// ・acceleration / deceleration が 0 以下なら加減速なし(目標速度が即座に乗る)。
// ・MovementComponent を持たない側は PositionIntegrationSystem が
//   目標速度をそのまま積分する(あちらは Exclude<MovementComponent>)。
//==============================================================================
void MovementIntegrationSystem::Init(Engine::ECS::World& a_world)
{
	a_world.ActiveTask<const VelocityComponent, MovementComponent, LocalTransformComponent>(
		Engine::ECS::ESystemType::Physics,
		"MovementIntegrationSystem",
		[](
			Engine::ECS::ArchetypeChunk* a_pChunk,
			uint32_t a_count,
			const Engine::ECS::SystemContext& a_ctx,
			ActiveTag* a_tags,
			const VelocityComponent* a_velocityArray,
			MovementComponent* a_movementArray,
			LocalTransformComponent* a_trsArray
		)
		{
			for (size_t _i = 0; _i < a_count; ++_i)
			{
				const VelocityComponent& _velComp = a_velocityArray[_i];
				MovementComponent& _moveComp = a_movementArray[_i];
				LocalTransformComponent& _trsComp = a_trsArray[_i];

				//----------------------------------------------------------
				// 水平: 目標速度へ加速度/減速度で寄せる
				//----------------------------------------------------------
				DXSM::Vector2 _current(_moveComp.velocity.x, _moveComp.velocity.z);
				DXSM::Vector2 _target(_velComp.value.x, _velComp.value.z);

				DXSM::Vector2 _diff   = _target - _current;
				float         _diffLen = _diff.Length();

				if (_diffLen > 1e-6f)
				{
					// 目標のほうが速ければ加速、遅ければ減速
					const float _rate = (_target.LengthSquared() >= _current.LengthSquared())
						? _moveComp.acceleration
						: _moveComp.deceleration;

					// 0 以下は加減速なし。1 フレームで詰めきる
					const float _step = (_rate > 0.0f) ? (_rate * a_ctx.dt) : _diffLen;

					// 行き過ぎないように残差でクランプする
					_current = (_step >= _diffLen)
						? _target
						: _current + _diff * (_step / _diffLen);
				}

				_moveComp.velocity.x = _current.x;
				_moveComp.velocity.z = _current.y;

				// 上下は重力/ジャンプ/ブーストの担当。そのまま通す
				_moveComp.velocity.y = _velComp.value.y;

				//----------------------------------------------------------
				// 積分
				//----------------------------------------------------------
				if (std::abs(_moveComp.velocity.x) > 0.0001f ||
					std::abs(_moveComp.velocity.y) > 0.0001f ||
					std::abs(_moveComp.velocity.z) > 0.0001f)
				{
					_trsComp.pos.x += _moveComp.velocity.x * a_ctx.dt;
					_trsComp.pos.y += _moveComp.velocity.y * a_ctx.dt;
					_trsComp.pos.z += _moveComp.velocity.z * a_ctx.dt;

					// 座標が変わったのでDirtyフラグを立てる
					_trsComp.isDirty = true;
				}
				else
				{
					// 微小な速度は残さずに止める
					_moveComp.velocity = {};
				}
			}
		}
	);
}
