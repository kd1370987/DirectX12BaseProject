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
//   戦闘中(isFind == true) : stopDistance(±keepMargin)の間合いを保つ。遠ければ詰め、
//                            近づかれすぎたら後ずさりし、間合いの内では止まる。
//                            同時に「最後に見た位置」を毎フレーム記録しておく。
//   離脱(戦闘 → 非戦闘)   : 最後に見た位置へ向かい(MoveTo)、着いたら見渡す(LookAround)。
//                            見渡し中に再び戦闘距離へ入らなければ徘徊へ戻る。
//   非戦闘(isFind == false): ランダム方向へ歩く → 立ち止まって首を振る、を繰り返す。
//
// ※ 戦闘に入る/抜ける判定は距離のみ(SearchPlayerSystem)。視界コーンは廃止した。
//
// ・方向の選択と「見失い探索のフェーズ進行」だけをここで行う。最終的な
//   「動けるか / 速度倍率」は FSM 側の canMove / moveSpeedScale を見て
//   ActionBehaviorSystem がゲートする。
// ・フェーズ(PatrolComponent.lostPhase)は
//     LostTargetBridgeSystem … FSM のパラメータ(LostSearch)へ橋渡し
//     LookAroundSystem       … 見失い探索中の旋回
//   が読む。どちらも PatrolComponent を読むだけなので本システムの後ろに回る。
// ・MoveIntent は敵の場合「世界空間の水平方向 × throttle(0..1)」を意味する
//   (プレイヤーはカメラ相対。消費側が別システムなので解釈を分けてよい)。
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
					// 見つけている間は探索フェーズを解除しておく
					// (再発見 → 追跡へ戻すのは FSM 側の SeePlayer が担当)
					_patrol.lostPhase = ELostPhase::None;
					_patrol.lostTimer = 0.0f;

					if (_target.targetEntity != Engine::ECS::Limits::INVALID_ENTITY &&
						a_ctx.pWorld->HasComponent<WorldMatrixComponent>(_target.targetEntity))
					{
						if (const auto* _pPlayerWorld =
							a_ctx.pWorld->RefData<WorldMatrixComponent>(_target.targetEntity))
						{
							DXSM::Vector3 _playerPos = DXSM::Matrix(_pPlayerWorld->worldMat).Translation();

							// 見失ったときに向かう地点として、見えている間ずっと更新しておく
							_patrol.lastSeenPos = { _playerPos.x, _playerPos.y, _playerPos.z };

							DXSM::Vector3 _to = _playerPos - DXSM::Vector3(_trs.pos);
							_to.y = 0.0f;	// 水平のみ

							float _lenSq = _to.LengthSquared();
							if (_lenSq > 1e-6f)
							{
								// 遠隔攻撃なので間合いを保つ(距離は SearchPlayer 計算済み)
								//   遠い → 詰める / 近い → 下がる / 間 → 止まる
								//
								// 保つ間合いは攻撃可能距離の内側であること。外で止まると
								// 永久に攻撃圏へ入れず、追従したまま撃たなくなる。
								// stopDistance がそれより外なら攻撃圏の内側へ寄せる。
								const float _keep = (std::max)(0.0f,
									(std::min)(_patrol.stopDistance, _target.attackDistance - _patrol.keepMargin));

								const float _far  = _keep + _patrol.keepMargin;
								const float _near = _keep - _patrol.keepMargin;

								if (_target.distance > _far)
								{
									_moveDir  = _to / std::sqrt(_lenSq);
									_throttle = _patrol.chaseThrottle;
								}
								else if (_target.distance < _near)
								{
									// プレイヤーを向いたまま後ずさりする(向きは FaceTargetSystem)
									_moveDir  = -_to / std::sqrt(_lenSq);
									_throttle = _patrol.backThrottle;
								}
							}
						}
					}
				}
				else
				{
					//----------------------------------------------------
					// 見失った瞬間(発見 → 未発見)に探索を開始する
					//----------------------------------------------------
					if (_patrol.wasFind)
					{
						_patrol.lostPhase = ELostPhase::MoveTo;
						_patrol.lostTimer = 0.0f;
					}

					if (_patrol.lostPhase == ELostPhase::MoveTo)
					{
						//------------------------------------------------
						// 最後に見た地点へ向かう
						//------------------------------------------------
						_patrol.lostTimer += a_ctx.dt;

						DXSM::Vector3 _to = DXSM::Vector3(_patrol.lastSeenPos) - DXSM::Vector3(_trs.pos);
						_to.y = 0.0f;
						float _lenSq = _to.LengthSquared();

						// 到着した / いつまでも辿り着けない(壁の向こうなど)なら見渡しへ
						const float _arrive = _patrol.lostArriveDistance;
						if (_lenSq <= (_arrive * _arrive) || _patrol.lostTimer >= _patrol.lostMoveTimeout)
						{
							_patrol.lostPhase = ELostPhase::LookAround;
							_patrol.lostTimer = 0.0f;

							// 見渡しの基準は「到着したときに向いている方向」
							// 左手系 +Z 前方: Yaw = atan2(x, z)
							DXSM::Vector3 _forward = DXSM::Vector3::Transform(
								DXSM::Vector3(0.0f, 0.0f, 1.0f), DXSM::Quaternion(_trs.quat));
							_patrol.lookAroundYaw = std::atan2(_forward.x, _forward.z);
						}
						else
						{
							_moveDir  = _to / std::sqrt(_lenSq);
							_throttle = _patrol.lostThrottle;
						}
					}
					else if (_patrol.lostPhase == ELostPhase::LookAround)
					{
						//------------------------------------------------
						// その場で見渡す(旋回は LookAroundSystem が行う)
						//------------------------------------------------
						_patrol.lostTimer += a_ctx.dt;

						if (_patrol.lostTimer >= _patrol.lookAroundTime)
						{
							// 見つけられなかった → 徘徊へ戻る
							_patrol.lostPhase   = ELostPhase::None;
							_patrol.lostTimer   = 0.0f;

							// さんざん見渡した後なので、立ち止まらずに
							// 次のフレームで方向を選び直して歩き出させる
							_patrol.patrolPhase = EPatrolPhase::Pause;
							_patrol.wanderTimer = 0.0f;
						}
					}
					else
					{
						//------------------------------------------------
						// 徘徊: 「ランダム方向へ歩く → 立ち止まって首を振る」の繰り返し
						// (首を振る旋回自体は LookAroundSystem が行う)
						//------------------------------------------------
						_patrol.wanderTimer -= a_ctx.dt;
						if (_patrol.wanderTimer <= 0.0f)
						{
							if (_patrol.patrolPhase == EPatrolPhase::Move)
							{
								// 歩き終わり → その場で立ち止まる
								// 首振りの基準は「立ち止まったときに向いている方向」
								DXSM::Vector3 _forward = DXSM::Vector3::Transform(
									DXSM::Vector3(0.0f, 0.0f, 1.0f), DXSM::Quaternion(_trs.quat));
								_patrol.patrolLookYaw = std::atan2(_forward.x, _forward.z);

								_patrol.patrolPhase = EPatrolPhase::Pause;
								_patrol.wanderTimer = _patrol.patrolPauseTime;
							}
							else
							{
								// 見終わり → 次の方向を選んで歩き出す
								float _a = s_angle(s_rng);
								_patrol.wanderDir = { std::sin(_a), 0.0f, std::cos(_a) };

								_patrol.patrolPhase = EPatrolPhase::Move;
								_patrol.wanderTimer = _patrol.retargetInterval;
							}
						}

						// 立ち止まっている間は動かない
						if (_patrol.patrolPhase == EPatrolPhase::Move)
						{
							_moveDir  = DXSM::Vector3(_patrol.wanderDir);
							_throttle = _patrol.patrolThrottle;
						}
					}
				}

				// 次フレームで「見失った瞬間」を拾うために今回の結果を覚えておく
				_patrol.wasFind = _target.isFind;

				// 世界空間の水平方向 × throttle を MoveIntent へ
				_intent.value.x = _moveDir.x * _throttle;
				_intent.value.y = 0.0f;
				_intent.value.z = _moveDir.z * _throttle;
			}
		}
	);
}
