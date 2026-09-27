#include "BoidSnapshotSystem.h"

#include "Application/ECS/World/APPWorld.h"

#include "Application/Components/Boid/BoidMembershipComponent.h"
#include "Application/Components/Transform/LocalTransformComponent.h"
#include "Application/Components/Movement/DesiredVelocityComponent.h"

#include "Application/InstanceResource/BoidSnapshotResource.h"

//==============================================================================
// BoidSnapshotSystem
//
// 群れの操舵の前段。全ボイドの位置と速度を小隊ごとに BoidSnapshotResource へ写す。
// 操舵(BoidSteeringSystem)はこの写しを近傍として読み、自分の速度だけを書く。
//
// ・全員の更新前の値で操舵するための写し。操舵の途中で書き換わった速度を
//   ほかのボイドが読むと、処理の順番で結果が変わってしまう。
// ・写すのは1回の走査だけなので、メインスレッドのカスタムタスクで回す
//   (小隊の配列へ積むのは、チャンクを分けて同時にやると取り合いになる)。
// ・PreUpdate 帯。速度の書き手(HomingSystem など)の後に並ぶのは読み書きの依存で決まる。
//==============================================================================
void BoidSnapshotSystem::Init(App::ECS::APPWorld& a_world)
{
	a_world.ActiveCustomTask(
		Engine::ECS::ESystemType::PreUpdate,
		"BoidSnapshotSystem",
		Engine::ECS::ReadList<BoidMembershipComponent, LocalTransformComponent, DesiredVelocityComponent>{},
		Engine::ECS::WriteList<>{},
		[](const Engine::ECS::SystemContext& a_ctx)
		{
			if (!a_ctx.pWorld) return;

			BoidSnapshotResource& _snapshot = a_ctx.pWorld->GetResource<BoidSnapshotResource>();
			_snapshot.Clear();

			a_ctx.pWorld->ForEach<
				const ActiveTag,
				const BoidMembershipComponent,
				const LocalTransformComponent,
				const DesiredVelocityComponent>(
					[&_snapshot](
						Engine::ECS::Chunk*,
						uint32_t a_count,
						const ActiveTag*,
						const BoidMembershipComponent* a_memberArray,
						const LocalTransformComponent* a_trsArray,
						const DesiredVelocityComponent* a_velArray
					)
					{
						for (uint32_t _i = 0; _i < a_count; ++_i)
						{
							// 小隊に属していないボイドは群体制御の対象外
							const Engine::ECS::Entity _platoonID = a_memberArray[_i].platoonID;
							if (_platoonID == Engine::ECS::Limits::INVALID_ENTITY) continue;

							_snapshot.Push(_platoonID, { a_trsArray[_i].pos, a_velArray[_i].value });
						}
					}
				);
		}
	)
	.WritesResource<BoidSnapshotResource>();
}
