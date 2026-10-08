#include "MovementIntegrationSystem.h"

#include "Application/ECS/World/APPWorld.h"

#include "Application/Components/Movement/DesiredVelocityComponent.h"
#include "Application/Components/Movement/MovementParamsComponent.h"
#include "Application/Components/Movement/ActualVelocityComponent.h"
#include "Application/Components/Transform/LocalTransformComponent.h"

namespace App::System
{
	//==============================================================================
	// MovementIntegrationSystem
	//
	// DesiredVelocityComponent(目標速度)へ MovementParamsComponent の加速度/減速度で追従させ、
	// その実速度(ActualVelocityComponent)で座標を進める。
	// 旧 InertiaIntegrationSystem(時定数による指数追従)の置き換え。
	//
	// ・加減速をかけるのは水平(XZ)だけ。上下は重力/ジャンプ/ブーストが直接作る値なので
	//   そのまま通す。加減速を挟むと落下や着地が鈍るため。
	// ・加速か減速かは「目標速度が今の速さより速いか」で選ぶ。向きを変えるだけで
	//   速さが変わらない旋回中は加速度側になる。
	// ・acceleration / deceleration が 0 以下なら加減速なし(目標速度が即座に乗る)。
	// ・MovementParamsComponent を持たない側は PositionIntegrationSystem が
	//   目標速度をそのまま積分する(あちらは Exclude<MovementParamsComponent>)。
	//==============================================================================
	void MovementIntegrationSystem::Init(App::ECS::APPWorld& a_world)
	{
		// 自分のチャンクの配列だけを書くので、チャンクを分けてワーカーで回す。
		// PositionIntegrationSystem とは対象のアーキタイプが重ならないので、同時に走る
		a_world.ActiveJobTask<const Component::DesiredVelocityComponent, const Component::MovementParamsComponent, Component::ActualVelocityComponent, Component::LocalTransformComponent>(
			Engine::ECS::ESystemType::Physics,
			"MovementIntegrationSystem",
			[](
				Engine::ECS::Chunk* /*a_pChunk*/,
				uint32_t a_count,
				const Engine::ECS::SystemContext& a_ctx,
				Component::ActiveTag* /*a_tags*/,
				const Component::DesiredVelocityComponent* a_velocityArray,
				const Component::MovementParamsComponent* a_movementArray,
				Component::ActualVelocityComponent* a_actualArray,
				Component::LocalTransformComponent* a_trsArray
			)
			{
				for (size_t _i = 0; _i < a_count; ++_i)
				{
					const Component::DesiredVelocityComponent& _velComp = a_velocityArray[_i];
					const Component::MovementParamsComponent& _moveComp = a_movementArray[_i];
					Math::Vector3& _actual = a_actualArray[_i].value;
					Component::LocalTransformComponent& _trsComp = a_trsArray[_i];

					//----------------------------------------------------------
					// 水平: 目標速度へ加速度/減速度で寄せる
					//----------------------------------------------------------
					Math::Vector2 _current(_actual.x, _actual.z);
					Math::Vector2 _target(_velComp.value.x, _velComp.value.z);

					Math::Vector2 _diff   = _target - _current;
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

					_actual.x = _current.x;
					_actual.z = _current.y;

					// 上下は重力/ジャンプ/ブーストの担当。そのまま通す
					_actual.y = _velComp.value.y;

					//----------------------------------------------------------
					// 積分
					//----------------------------------------------------------
					if (std::abs(_actual.x) > 0.0001f ||
						std::abs(_actual.y) > 0.0001f ||
						std::abs(_actual.z) > 0.0001f)
					{
						_trsComp.pos.x += _actual.x * a_ctx.dt;
						_trsComp.pos.y += _actual.y * a_ctx.dt;
						_trsComp.pos.z += _actual.z * a_ctx.dt;

						// 座標が変わったのでDirtyフラグを立てる
						_trsComp.isDirty = true;
					}
					else
					{
						// 微小な速度は残さずに止める
						_actual = {};
					}
				}
			}
		)
		// 順序 : 座標(LocalTransform)の書き手同士(加減速の有無で対象は重ならない)
		.After("PositionIntegrationSystem");
	}
}
