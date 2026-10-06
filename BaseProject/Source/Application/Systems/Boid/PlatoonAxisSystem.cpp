#include "PlatoonAxisSystem.h"

#include "Application/ECS/World/APPWorld.h"

#include "Application/Components/Boid/PlatoonLeaderComponent.h"
#include "Application/Components/Transform/LocalTransformComponent.h"
#include "Application/Components/Movement/LookAngleComponent.h"

#include "Application/InstanceResource/PlatoonAxisResource.h"

//==============================================================================
// PlatoonAxisSystem
//
// 小隊長ごとの位置・進んでいる向き・頭からの1次元位置を PlatoonAxisResource へまとめる。
// 読むのは BoidWaveSystem(ボイドの1次元位置を出すのに使う)。
//
// ・以前は BoidWaveSystem がチャンクごとに ForEach で小隊長を集め直していた。
//   小隊長は数十なので、1回だけ集めて引けるようにする。
// ・前方は LookAngleComponent から作る(SwarmLookSystem が進行方向へ寄せている値)。
//   速度から直に作らないのは、止まった瞬間に向きが決まらなくなるのを避けるため。
// ・Update 帯。小隊長の向きと位置を書くもの(SwarmLookSystem など)の後に並ぶのは、
//   読み書きの依存で決まる。
//==============================================================================
void PlatoonAxisSystem::Init(App::ECS::APPWorld& a_world)
{
	a_world.ActiveCustomTask(
		Engine::ECS::ESystemType::Update,
		"PlatoonAxisSystem",
		Engine::ECS::ReadList<PlatoonLeaderComponent, LocalTransformComponent, LookAngleComponent>{},
		Engine::ECS::WriteList<>{},
		[](const Engine::ECS::SystemContext& a_ctx)
		{
			if (!a_ctx.pWorld) return;

			PlatoonAxisResource& _axisRes = a_ctx.pWorld->RefResource<PlatoonAxisResource>();
			_axisRes.Clear();

			a_ctx.pWorld->ForEach<const ActiveTag, const PlatoonLeaderComponent,
				const LocalTransformComponent, const LookAngleComponent>(
				[&_axisRes](
					Engine::ECS::Chunk* a_pChunk,
					uint32_t a_count,
					const ActiveTag*,
					const PlatoonLeaderComponent* a_platoonArray,
					const LocalTransformComponent* a_trsArray,
					const LookAngleComponent* a_lookArray
				)
				{
					for (uint32_t _i = 0; _i < a_count; ++_i)
					{
						PlatoonAxisResource::Axis& _axis = _axisRes.axisMap[a_pChunk->entityData[_i]];
						_axis.pos               = a_trsArray[_i].pos;
						_axis.forward           = MakeLookForward(a_lookArray[_i]);
						_axis.distanceAlongWorm = a_platoonArray[_i].distanceAlongWorm;
					}
				}
			);
		}
	)
	.WritesResource<PlatoonAxisResource>();
}
