#include "EnemyMovementSystem.h"

#include "Engine/ECS/World/World.h"

#include "../../../../../Components/Intent/MoveIntentComponent.h"
#include "../../../../../Components/Character/PatrolComponent.h"
#include "../../../../../Components/Force/VelocityComponent.h"
#include "../../../../../Components/Force/MovementComponent.h"

//==============================================================================
// EnemyMovementSystem
//
// 敵の MoveIntent(世界空間の水平方向 × throttle)を水平速度へ変換する。
// プレイヤー用の CharacterMovementSystem は LookAngleComponent 必須
// (カメラ相対)なので敵には効かない。その敵版。
//
// ・PatrolComponent を持つ = 敵、というクエリで対象を絞る。
// ・移動速度は MovementComponent.moveSpeed(プレイヤーと同じ置き場)。
//   加速度/減速度で実速度へ均すのは MovementIntegrationSystem の担当。
// ・y(重力/ジャンプ)は触らない。GravitySystem に任せる。
// ・この後 ActionBehaviorSystem が現在ステートの canMove / moveSpeedScale で
//   水平速度をゲート・スケールするので、本システムは Update 帯で
//   ActionBehaviorSystem より前に登録すること。
//==============================================================================
void EnemyMovementSystem::Init(Engine::ECS::World& a_world)
{
	a_world.ActiveTask<const MoveIntentComponent, const PatrolComponent, const MovementComponent,
		VelocityComponent>(
		Engine::ECS::ESystemType::Update,
		"EnemyMovementSystem",
		[](
			Engine::ECS::ArchetypeChunk*      a_pChunk,
			uint32_t                          a_count,
			const Engine::ECS::SystemContext& a_ctx,
			ActiveTag*                        a_tags,
			const MoveIntentComponent*        a_intentArray,
			const PatrolComponent*            a_patrolArray,
			const MovementComponent*          a_movementArray,
			VelocityComponent*                a_velArray
		)
		{
			for (size_t _i = 0; _i < a_count; ++_i)
			{
				const MoveIntentComponent& _intent = a_intentArray[_i];
				const MovementComponent&   _move   = a_movementArray[_i];
				VelocityComponent&         _vel    = a_velArray[_i];

				// 世界空間の水平方向 × throttle × moveSpeed
				_vel.value.x = _intent.value.x * _move.moveSpeed;
				_vel.value.z = _intent.value.z * _move.moveSpeed;
				// y は重力に任せる
			}
		}
	);
}
