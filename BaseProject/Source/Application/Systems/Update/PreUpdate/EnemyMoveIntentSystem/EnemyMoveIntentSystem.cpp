#include "EnemyMoveIntentSystem.h"

#include "Engine/ECS/World/World.h"

#include "../../../../Components/Character/TargetEntityComponent.h"
#include "../../../../Components/Character/PatrolComponent.h"
#include "../../../../Components/Intent/MoveIntentComponent.h"
#include "../../../../Components/Transform/LocalTransformComponent.h"
#include "../../../../Components/Transform/WorldMatrixComponent.h"

#include <random>

//==============================================================================
// EnemyMoveIntentSystem
//
// 敵の「行動決定」をプレイヤーの入力と同じ帯(PreUpdate)で行い、結果を
// MoveIntentComponent(世界空間・水平)へ書く。プレイヤーの InputMoveSystem に相当。
//
//   未発見(isFind == false): 一定間隔でランダム方向を選び、その辺を徘徊する。
//   発見(isFind == true)   : プレイヤー方向へ進む。stopDistance 以下なら停止(攻撃間合い)。
//
// ・方向の選択だけをここで行う。最終的な「動けるか / 速度倍率」は FSM 側の
//   canMove / moveSpeedScale を見て ActionBehaviorSystem がゲートする。
// ・MoveIntent は敵の場合「世界空間の水平方向 × throttle(0..1)」を意味する
//   (プレイヤーはカメラ相対。消費側が別системなので解釈を分けてよい)。
//   実速度への変換は EnemyMovementSystem が行う。
//==============================================================================
void EnemyMoveIntentSystem::Init(Engine::ECS::World& a_world)
{
	a_world.ActiveTask<const TargetEntityComponent, PatrolComponent, const LocalTransformComponent, MoveIntentComponent>(
		Engine::ECS::ESystemType::PreUpdate,
		"EnemyMoveIntentSystem",
		[](
			Engine::ECS::ArchetypeChunk*      a_pChunk,
			uint32_t                          a_count,
			const Engine::ECS::SystemContext& a_ctx,
			ActiveTag*                        a_tags,
			const TargetEntityComponent*      a_targetArray,
			PatrolComponent*                  a_patrolArray,
			const LocalTransformComponent*    a_trsArray,
			MoveIntentComponent*              a_intentArray
		)
		{
			// 徘徊方向の乱数(角度)。プロセス内で 1 つ持てば十分
			static std::mt19937 s_rng{ std::random_device{}() };
			static std::uniform_real_distribution<float> s_angle(0.0f, DirectX::XM_2PI);

			for (size_t _i = 0; _i < a_count; ++_i)
			{
				const TargetEntityComponent& _target = a_targetArray[_i];
				PatrolComponent&             _patrol = a_patrolArray[_i];
				const LocalTransformComponent& _trs  = a_trsArray[_i];
				MoveIntentComponent&         _intent = a_intentArray[_i];

				DXSM::Vector3 _moveDir = {};	// 世界・水平・単位
				float         _throttle = 0.0f;	// 0..1

				if (_target.isFind)
				{
					//----------------------------------------------------
					// 追跡: プレイヤー方向へ。近づきすぎたら止まる
					//----------------------------------------------------
					if (_target.targetEntity != Engine::ECS::Limits::INVALID_ENTITY &&
						a_ctx.pWorld->HasComponent<WorldMatrixComponent>(_target.targetEntity))
					{
						if (const auto* _pPlayerWorld =
							a_ctx.pWorld->RefData<WorldMatrixComponent>(_target.targetEntity))
						{
							DXSM::Vector3 _playerPos = DXSM::Matrix(_pPlayerWorld->worldMat).Translation();

							DXSM::Vector3 _to = _playerPos - DXSM::Vector3(_trs.pos);
							_to.y = 0.0f;	// 水平のみ

							float _lenSq = _to.LengthSquared();
							// stopDistance より遠いときだけ前進(距離は SearchPlayer 計算済み)
							if (_lenSq > 1e-6f && _target.distance > _patrol.stopDistance)
							{
								_moveDir  = _to / std::sqrt(_lenSq);
								_throttle = _patrol.chaseThrottle;
							}
						}
					}
				}
				else
				{
					//----------------------------------------------------
					// 徘徊: 一定間隔でランダムに方向転換して歩き回る
					//----------------------------------------------------
					_patrol.wanderTimer -= a_ctx.dt;
					if (_patrol.wanderTimer <= 0.0f)
					{
						float _a = s_angle(s_rng);
						_patrol.wanderDir = { std::sin(_a), 0.0f, std::cos(_a) };
						_patrol.wanderTimer = _patrol.retargetInterval;
					}

					_moveDir  = DXSM::Vector3(_patrol.wanderDir);
					_throttle = _patrol.patrolThrottle;
				}

				// 世界空間の水平方向 × throttle を MoveIntent へ
				_intent.value.x = _moveDir.x * _throttle;
				_intent.value.y = 0.0f;
				_intent.value.z = _moveDir.z * _throttle;
			}
		}
	);
}
