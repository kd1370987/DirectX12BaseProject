#include "BoidSystem.h"

#include "Application/ECS/World/APPWorld.h"

#include "../../../../Components/Character/BoidComponent.h"
#include "../../../../Components/Transform/LocalTransformComponent.h"
#include "../../../../Components/Intent/MoveIntentComponent.h"
#include "../../../../Components/Force/VelocityComponent.h"

//==============================================================================
// BoidSystem
//
// 群れの操舵(分離・整列・結合・目標への追従)から目標速度を作る。
//
// ・読むのは BoidComponent / LocalTransform / Velocity、書くのは Velocity だけ。
//   位置を進めるのは MovementIntegrationSystem(Physics)なので LocalTransform は書かない。
//   以前は LocalTransform を書き込みに挙げていて、使ってもいない辺が
//   BoidWaveSystem などとの間で依存の循環を作っていた。
// ・PreUpdate 帯で回す(HomingSystem と同じ理由)。
//   Update 帯には LockOnRotationSystem など「Velocity を読んで LocalTransform を書く」ものがいて、
//   こちらは「LocalTransform を読んで Velocity を書く」ので、同じ帯に置くと依存が循環する。
//   相手はプレイヤーでボイドとは別のエンティティだが、依存のグラフは型単位でしか見ないため。
//   目標速度を決めるだけの処理なので、敵の行動決定と同じ帯が素直でもある。
//   ボイドの位置と速度は Update 帯では誰も書かない(位置は Physics の積分)ので、
//   Update に置いていたときと同じ値を読む。目標地点を書く FollowLeaderSystem も PreUpdate。
//==============================================================================
void BoidSystem::Init(App::ECS::APPWorld& a_world)
{
	a_world.ActiveCustomJobTask(
		Engine::ECS::ESystemType::PreUpdate,
		"BoidSystem",
		Engine::ECS::ReadList<BoidComponent, LocalTransformComponent, VelocityComponent>{},
		Engine::ECS::WriteList<VelocityComponent>{},
		[](const Engine::ECS::SystemContext& a_ctx)
		{
			ENGINE_PROFILE_SCOPE("BoidSystem");

			if (!a_ctx.pWorld) return;
			// ============================================================ 
			// 近傍検索用の一時データ 
			// 
			// 群体制御は同じ小隊のBoid同士だけで行うので、小隊長ごとの配列に分けて持つ。
			// 各Boidは自分の小隊の配列だけを見ればよく、他の小隊の分を読み飛ばさずに済む。
			// 
			// 小隊の中は現在はテスト段階なので O(N^2) で近傍検索する。 
			// 小隊が大きくなった段階ではSpatial Hash / Grid / BVHなどへ 
			// 置き換えることを想定。 
			// ============================================================
			struct BoidData
			{
				Math::Vector3 position;
				Math::Vector3 velocity;
			};
			// 小隊長 -> 所属Boidの配列
			std::unordered_map<Engine::ECS::Entity, std::vector<BoidData>> _platoonBoidMap = {};

			// ============================================================ 
			// 全Boidの現在状態をスナップショットとして取得する 
			// ここで取得した状態を使って全Boidを更新することで
			// 更新順によって結果が変わることを防ぐ。 
			// ============================================================
			a_ctx.pWorld->ForEach<
				const ActiveTag,
				const BoidComponent,
				const LocalTransformComponent,
				const VelocityComponent>(
					[&_platoonBoidMap](
						Engine::ECS::Chunk* a_pChunk,
						uint32_t a_count,
						const ActiveTag* a_tags,
						const BoidComponent* a_boidArray,
						const LocalTransformComponent* a_localTRSArray,
						const VelocityComponent* a_velArray
						)
					{
						for (size_t _i = 0; _i < a_count; ++_i)
						{
							// 小隊に属していないBoidは群体制御の対象外
							const Engine::ECS::Entity _platoonID = a_boidArray[_i].platoonID;
							if (_platoonID == Engine::ECS::Limits::INVALID_ENTITY) continue;

							_platoonBoidMap[_platoonID].push_back(
								{ a_localTRSArray[_i].pos,a_velArray[_i].value }
							);
						}
					}
				);

			// ============================================================ 
			// 各Boidを更新 
			// ============================================================
			a_ctx.pWorld->ForEach<const ActiveTag, const BoidComponent, const LocalTransformComponent,VelocityComponent>(
				[&_platoonBoidMap,&a_ctx](
					Engine::ECS::Chunk* a_pChunk,
					uint32_t a_count,
					const ActiveTag* a_tags,
					const BoidComponent* a_boidArray,
					const LocalTransformComponent* a_localTRSArray,
					VelocityComponent* a_velArray
					)
				{
					for (size_t _i = 0; _i < a_count; ++_i)
					{
						const BoidComponent& _boidComp = a_boidArray[_i];
						const LocalTransformComponent& _trsComp = a_localTRSArray[_i];
						VelocityComponent& _velComp = a_velArray[_i];

						
						Math::Vector3 _separation;// すべての反発を合成する
						Math::Vector3 _alignment{};// 周囲の進行ベクトル
						Math::Vector3 _cohesion{};	// ボイドの中心

						uint32_t _neighborCount = 0;

						// ============================================================ 
						// 同じ小隊のボイドを検索
						// 
						// 小隊に属していなければ近傍は無し(Seekだけ効く)
						// ============================================================
						std::span<const BoidData> _platoonBoidSpan = {};
						if (auto _it = _platoonBoidMap.find(_boidComp.platoonID); _it != _platoonBoidMap.end())
						{
							_platoonBoidSpan = _it->second;
						}

						for (const auto& _other : _platoonBoidSpan)
						{
							Math::Vector3 _offset = _trsComp.pos - _other.position;
							float _distanceSquared = _offset.LengthSquared();

							// 同一座標の場合は方向を求められないので無視
							if (_distanceSquared <= 0.000001f) continue;
							const float _distance = std::sqrt(_distanceSquared);

							// -------------------------------------------------------------
							// Separation
							// 
							// 近すぎるボイドを押し返す、separationDistanceを超えたボイドは
							// 反発力を受けない
							// -------------------------------------------------------------
							if (_distance < _boidComp.separationDistance)
							{
								// 反発方向
								const Math::Vector3 _direction = _offset / _distance;

								// 近いほど１に近づく。separationDistanceに近づけば0になる
								const float _ratio = (_boidComp.separationDistance - _distance) / _boidComp.separationDistance;

								// 少し近い場合は弱く、極端に近ければ強く押し返す
								_separation += _direction * (_ratio * _ratio);
							}

							// -------------------------------------------------------------
							// Alignment / Cohesion
							// 
							// Separationとは別の範囲を使用する
							// Separation : 近すぎる個体を押し返す
							// Neighbor : 周囲の個体と軍隊として行動する
							// -------------------------------------------------------------
							if (_distance < _boidComp.distanceLenge)
							{
								_alignment += _other.velocity;
								_cohesion += _other.position;
								++_neighborCount;
							}
						}
						// -------------------------------------------------------------
						// Alignment / Cohesion を平均化
						// -------------------------------------------------------------
						if (_neighborCount > 0)
						{
							const float _neighborCountInv = 1.0f / static_cast<float>(_neighborCount);

							// Alignment 周囲の平均速度との差を求める
							_alignment *= _neighborCountInv;
							_alignment -= _velComp.value;

							// Cohesion 周囲の平均位置へ向かう方向を求める
							_cohesion *= _neighborCountInv;
							_cohesion -= _trsComp.pos;
						}

						// -------------------------------------------------------------
						// Steering を合成
						// -------------------------------------------------------------
						Math::Vector3 _steering = {};

						// Separation 
						_steering += _separation * _boidComp.separationWeight;
						
						// Alignment
						_steering += _alignment * _boidComp.alignmentWeight;

						// Cohesion
						_steering += _cohesion * _boidComp.cohesionWeight;

						// -------------------------------------------------------------
						// Seek
						// 
						// 目標地点へ向かうべき速度と現在速度との差から作る
						// -------------------------------------------------------------
						Math::Vector3 _toTarget = _boidComp.targetPos - _trsComp.pos;
						const float _targetDistance = _toTarget.Length();
						
						// 目標地点から離れていれば近づく
						if (_targetDistance > 0.001f)
						{
							_toTarget /= _targetDistance;

							// 目標地点付近では減速する
							const float _slowRadius = _boidComp.slowRadius;
							float _targetSpeed = _boidComp.maxSpeed;

							if (_targetDistance < _slowRadius)
							{
								_targetSpeed *= _targetDistance / _slowRadius;
							}

							const Math::Vector3 _targetVelocity = _toTarget * _targetSpeed;

							// 現在速度との差をSteeringとして扱う
							const Math::Vector3 _seek = _targetVelocity - _velComp.value;

							_steering += _seek * _boidComp.seekWeight;

						}

						// -------------------------------------------------------------
						// Steeringの最大値を制限
						// 
						// Separation / Cohesion / Seek に上限を設けて吹き飛ぶ挙動を阻止
						// -------------------------------------------------------------
						const float _steeringLengthSquared = _steering.LengthSquared();
						if (_steeringLengthSquared > _boidComp.maxSteeringForce * _boidComp.maxSteeringForce)
						{
							_steering.Normalize();
							_steering *= _boidComp.maxSteeringForce;
						}

						// -------------------------------------------------------------
						// Velocity更新
						// -------------------------------------------------------------
						_velComp.value += _steering * a_ctx.dt;

						// -------------------------------------------------------------
						// 最大速度制限
						// 
						// Seek だけでなくほかの Steeringによって速度が上がるため
						// 最終的な速度にも制限
						// -------------------------------------------------------------
						const float _velocityLengthSquared = _velComp.value.LengthSquared();

						if (_velocityLengthSquared > _boidComp.maxSpeed * _boidComp.maxSpeed)
						{
							_velComp.value.Normalize();
							_velComp.value *= _boidComp.maxSpeed;
						}
					}
				}
			);
		}
	);
}
