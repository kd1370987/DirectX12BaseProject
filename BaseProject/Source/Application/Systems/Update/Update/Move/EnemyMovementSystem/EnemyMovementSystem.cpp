#include "EnemyMovementSystem.h"

#include "Engine/ECS/World/World.h"

#include "../../../../../Components/Intent/MoveIntentComponent.h"
#include "../../../../../Components/Character/PatrolComponent.h"
#include "../../../../../Components/Force/VelocityComponent.h"

//==============================================================================
// EnemyMovementSystem
//
// 敵の MoveIntent(世界空間の水平方向 × throttle)を水平速度へ変換する。
// プレイヤー用の CharacterMovementSystem は PlayerLookAngleComponent 必須
// (カメラ相対)なので敵には効かない。その敵版。
//
// ・PatrolComponent を持つ = 敵、というクエリで対象を絞る。
// ・y(重力/ジャンプ)は触らない。GravitySystem に任せる。
// ・この後 ActionBehaviorSystem が現在ステートの canMove / moveSpeedScale で
//   水平速度をゲート・スケールするので、本システムは Update 帯で
//   ActionBehaviorSystem より前に登録すること。
//==============================================================================
void EnemyMovementSystem::Init(Engine::ECS::World& a_world)
{
	a_world.ActiveTask<const MoveIntentComponent, const PatrolComponent, VelocityComponent>(
		Engine::ECS::ESystemType::Update,
		"EnemyMovementSystem",
		[](
			Engine::ECS::ArchetypeChunk*      a_pChunk,
			uint32_t                          a_count,
			const Engine::ECS::SystemContext& a_ctx,
			ActiveTag*                        a_tags,
			const MoveIntentComponent*        a_intentArray,
			const PatrolComponent*            a_patrolArray,
			VelocityComponent*                a_velArray
		)
		{
			for (size_t _i = 0; _i < a_count; ++_i)
			{
				const MoveIntentComponent& _intent = a_intentArray[_i];
				const PatrolComponent&     _patrol = a_patrolArray[_i];
				VelocityComponent&         _vel    = a_velArray[_i];

				// 世界空間の水平方向 × throttle × maxSpeed
				_vel.value.x = _intent.value.x * _patrol.maxSpeed;
				_vel.value.z = _intent.value.z * _patrol.maxSpeed;
				// y は重力に任せる
			}
		}
	);
}
