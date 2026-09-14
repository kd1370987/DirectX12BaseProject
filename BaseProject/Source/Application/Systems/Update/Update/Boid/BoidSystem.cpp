#include "BoidSystem.h"

#include "Application/ECS/World/APPWorld.h"

#include "../../../../Components/Character/BoidComponent.h"
#include "../../../../Components/Transform/LocalTransformComponent.h"
#include "../../../../Components/Intent/MoveIntentComponent.h"
#include "../../../../Components/Force/VelocityComponent.h"

void BoidSystem::Init(App::ECS::APPWorld& a_world)
{
	a_world.ActiveCustomTask(
		Engine::ECS::ESystemType::Update,
		Engine::ECS::ReadList<BoidComponent,VelocityComponent>{},
		Engine::ECS::WriteList<LocalTransformComponent,VelocityComponent>{},
		[](const Engine::ECS::SystemContext& a_ctx)
		{
			if (!a_ctx.pWorld) return;
			struct BoidData
			{
				Math::Vector3 position;
				Math::Vector3 velocity;
			};
			// 全ボイドの居場所を取得 : テスト段階なため全走査
			std::vector<BoidData> _boidPosVec = {};
			a_ctx.pWorld->ForEach<
				const ActiveTag,
				const BoidComponent,
				const LocalTransformComponent,
				const VelocityComponent>(
					[&_boidPosVec](
						Engine::ECS::ArchetypeChunk* a_pChunk,
						uint32_t a_count,
						const ActiveTag* a_tags,
						const BoidComponent* a_boidArray,
						const LocalTransformComponent* a_localTRSArray,
						const VelocityComponent* a_velArray
						)
					{
						for (size_t _i = 0; _i < a_count; ++_i)
						{
							_boidPosVec.push_back({
								a_localTRSArray[_i].pos,
								a_velArray[_i].value
								});
						}
					}
				);

			// ボイドごとにディスタンスを確保
			a_ctx.pWorld->ForEach<const ActiveTag, const BoidComponent, const LocalTransformComponent,VelocityComponent>(
				[&_boidPosVec,&a_ctx](
					Engine::ECS::ArchetypeChunk* a_pChunk,
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

						for (auto& _boid : _boidPosVec)
						{
							Math::Vector3 _offset = _trsComp.pos - _boid.position;
							float _length = _offset.Length();

							if (_length < _boidComp.distanceLenge)
							{
								if (_length > 0)
								{
									_offset.Normalize();

									// 反発
									_separation += _offset / _length;

									// アライメント
									_alignment += _boid.velocity;
									++_neighborCount;

									// Cohesion
									_cohesion += _boid.position;

									++_neighborCount;
								}
							}
						}

						if (_neighborCount > 0)
						{
							_alignment /= static_cast<float>(_neighborCount);
							_alignment -= _velComp.value;

							_cohesion /= static_cast<float>(_neighborCount);
							_cohesion -= _trsComp.pos;
						}

						Math::Vector3 _steering = {};

						// 反発があれば移動方向として記録
						if (_separation.LengthSquared() > 0.0f)
						{
							_separation.Normalize();
							_steering += _separation * 2.0f;
						}

						_steering += _alignment * 1.0f;
						_steering += _cohesion * 0.5f;

						_steering += Math::Vector3(0,0,1);

						// ベロシティ更新
						_velComp.value += _steering * a_ctx.dt;
					}
				}
			);
		}
	);
}
