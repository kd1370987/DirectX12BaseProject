#include "SwarmLeaderMoveSystem.h"

#include "Application/ECS/World/APPWorld.h"

#include "Application/Components/Boid/BoidLeaderComponent.h"
#include "Application/Components/Movement/MoveIntentComponent.h"
#include "Application/Components/Movement/MovementParamsComponent.h"
#include "Application/Components/Movement/DesiredVelocityComponent.h"

namespace App::System
{
	//==============================================================================
	// SwarmLeaderMoveSystem
	//
	// 群れのリーダーの移動入力(MoveIntentComponent)を目標速度へ変える。
	// 入力を作るのは SwarmBossController(このボスの脳)で、ここは変換専門。
	//
	//   目標速度 = 移動入力(世界空間の向き × スロットル) × moveSpeed
	//
	// ・プレイヤー用の CharacterMovementSystem は視点基準(入力を Yaw で回す)なので使えない。
	//   ザコ用の EnemyMovementSystem は PatrolComponent が要り、上下も触らない。
	//   空を泳ぐリーダーは上下にも動くので、入力をそのまま世界空間として扱う。
	// ・上下も書く。MovementIntegrationSystem は上下に加減速を掛けず目標速度を素通しするので、
	//   入力が急に変わるとそのぶん速度も跳ねる(重力を持たせていないので問題にならない)。
	// ・実際に座標を進めるのは MovementIntegrationSystem(Physics)。
	//==============================================================================
	void SwarmLeaderMoveSystem::Init(App::ECS::APPWorld& a_world)
	{
		// 自分のチャンクの値だけを書く(ほかのエンティティは RefData で読むだけ)ので、ワーカーで回す
		a_world.ActiveJobTask<const Component::BoidLeaderComponent, const Component::MoveIntentComponent, const Component::MovementParamsComponent,
			Component::DesiredVelocityComponent>(
			Engine::ECS::ESystemType::Update,
			"SwarmLeaderMoveSystem",
			[](
				Engine::ECS::Chunk*      a_pChunk,
				uint32_t                          a_count,
				const Engine::ECS::SystemContext& a_ctx,
				Component::ActiveTag*                        a_tags,
				const Component::BoidLeaderComponent*        a_leaderArray,
				const Component::MoveIntentComponent*        a_intentArray,
				const Component::MovementParamsComponent*          a_movementArray,
				Component::DesiredVelocityComponent*                a_velArray
			)
			{
				for (size_t _i = 0; _i < a_count; ++_i)
				{
					const Component::MoveIntentComponent& _intent = a_intentArray[_i];
					const Component::MovementParamsComponent&   _move   = a_movementArray[_i];

					a_velArray[_i].value = _intent.value * _move.moveSpeed;
				}
			}
		);
	}
}
