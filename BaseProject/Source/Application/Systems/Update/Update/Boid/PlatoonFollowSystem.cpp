#include "PlatoonFollowSystem.h"

#include "Application/ECS/World/APPWorld.h"

#include "../../../../Components/Character/Boss/PlatoonLeaderComponent.h"
#include "../../../../Components/Transform/LocalTransformComponent.h"
#include "../../../../Components/Force/MovementComponent.h"
#include "../../../../Components/Force/VelocityComponent.h"

//==============================================================================
// PlatoonFollowSystem
//
// 小隊長を一つ前の相手(preLeader)へ distance の間隔をあけて追従させる。
// 書くのは目標速度(VelocityComponent)だけで、加減速と座標の積分は
// MovementIntegrationSystem(Physics)に任せる。
//
//   目標速度 = 前の相手の実速度 + 前の相手への向き × (距離 - distance) × followGain
//
// ・前の相手の実速度を上乗せするのは、ずれが出てから追いかけ始めると
//   間隔が開いたまま詰まらないため。止まっている相手なら間隔のずれだけで動く。
// ・近すぎるときはずれが負になるので、相手から離れる向きに下がる。
// ・速さは MovementComponent.moveSpeed で頭打ちにする。前の相手より遅いと離されていく。
// ・上下も同じ式で追う(地中や空中を潜って追いかけるため)。上下は加減速が掛からず
//   目標速度がそのまま乗る点に注意(MovementIntegrationSystem の仕様)。
//
// ・前の相手の位置と実速度は RefData で引く。どちらも前フレームの Physics の結果なので、
//   列の中で誰から先に処理しても同じ値を見る(並び順に依存しない)。
// ・自分の座標もクエリに入れず RefData で引く。BoidSystem(同じ Update 帯)が
//   LocalTransform を write / Velocity を read で宣言しているので、こちらが
//   LocalTransform を read / Velocity を write で並べると依存が輪になる。
// ・前の相手が居なくなったら目標速度を 0 にして止める。
//==============================================================================
void PlatoonFollowSystem::Init(App::ECS::APPWorld& a_world)
{
	a_world.ActiveTask<const PlatoonLeaderComponent, const MovementComponent, VelocityComponent>(
		Engine::ECS::ESystemType::Update,
		"PlatoonFollowSystem",
		[](
			Engine::ECS::ArchetypeChunk*      a_pChunk,
			uint32_t                          a_count,
			const Engine::ECS::SystemContext& a_ctx,
			ActiveTag*                        a_tags,
			const PlatoonLeaderComponent*     a_platoonArray,
			const MovementComponent*          a_movementArray,
			VelocityComponent*                a_velArray
		)
		{
			auto& _world = *a_ctx.pWorld;

			for (size_t _i = 0; _i < a_count; ++_i)
			{
				const PlatoonLeaderComponent&  _platoon = a_platoonArray[_i];
				const MovementComponent&       _move    = a_movementArray[_i];
				VelocityComponent&             _vel     = a_velArray[_i];

				const Engine::ECS::Entity _self = a_pChunk->entityData[_i];
				if (!_world.HasComponent<LocalTransformComponent>(_self)) continue;

				//----------------------------------------------------------
				// 前の相手が居なければ止まる
				//----------------------------------------------------------
				const Engine::ECS::Entity _pre = _platoon.preLeader;
				if (_pre == Engine::ECS::Limits::INVALID_ENTITY ||
					!_world.IsAliveEntity(_pre) ||
					!_world.HasComponent<LocalTransformComponent>(_pre))
				{
					_vel.value = Math::Vector3(0.0f, 0.0f, 0.0f);
					continue;
				}

				const Math::Vector3 _selfPos = _world.RefData<LocalTransformComponent>(_self)->pos;
				const Math::Vector3 _prePos  = _world.RefData<LocalTransformComponent>(_pre)->pos;

				// 前の相手の実速度。MovementComponent を持たなければ目標速度で代用する
				Math::Vector3 _preVel = {};
				if (_world.HasComponent<MovementComponent>(_pre))
				{
					_preVel = _world.RefData<MovementComponent>(_pre)->velocity;
				}
				else if (_world.HasComponent<VelocityComponent>(_pre))
				{
					_preVel = _world.RefData<VelocityComponent>(_pre)->value;
				}

				//----------------------------------------------------------
				// 間隔のずれを詰める速度を足す
				//----------------------------------------------------------
				Math::Vector3 _target = _preVel;

				const Math::Vector3 _toPre = _prePos - _selfPos;
				const float _dist = _toPre.Length();
				if (_dist > 1e-4f)
				{
					const float _gap = _dist - _platoon.distance;
					_target += (_toPre / _dist) * (_gap * _platoon.followGain);
				}

				// 速さの頭打ち
				if (_move.moveSpeed > 0.0f)
				{
					const float _speedSq = _target.LengthSquared();
					if (_speedSq > _move.moveSpeed * _move.moveSpeed)
					{
						_target *= _move.moveSpeed / std::sqrt(_speedSq);
					}
				}

				_vel.value = _target;
			}
		}
	);
}
