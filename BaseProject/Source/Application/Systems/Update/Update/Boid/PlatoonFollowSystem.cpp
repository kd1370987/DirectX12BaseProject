#include "PlatoonFollowSystem.h"

#include "Application/ECS/World/APPWorld.h"

#include "../../../../Components/Character/Boss/PlatoonLeaderComponent.h"
#include "../../../../Components/Transform/LocalTransformComponent.h"
#include "../../../../Components/Force/MovementComponent.h"
#include "../../../../Components/Force/VelocityComponent.h"
#include "../../../../Components/Character/LookAngleComponent.h"

//==============================================================================
// PlatoonFollowSystem
//
// 小隊長を一つ前の相手(preLeader)の**後ろ**へ追従させる。
// 書くのは目標速度(VelocityComponent)だけで、加減速と座標の積分は
// MovementIntegrationSystem(Physics)に任せる。
//
//   目標地点 = 前の相手の位置 - 前の相手の前方 × distance
//   目標速度 = 前の相手の実速度 + 目標地点への差 × followGain
//
// ・前の相手の「前方」は LookAngleComponent(SwarmLookSystem が進んでいる向きへ寄せる)。
//   角度を持っていない相手は体の向き(quat の +Z)で代用する。
// ・前の相手の実速度を上乗せするのは、ずれが出てから追いかけ始めると
//   間隔が開いたまま詰まらないため。止まっている相手なら位置のずれだけで動く。
// ・目標地点の手前でも奥でも同じ式で寄る(追い越したら下がる)。
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
namespace
{
	//--------------------------------------------------------------------------
	// そのエンティティが向いている方向
	//
	// 視点角(LookAngleComponent)を持っているならそこから作る。
	// 持っていない相手は体の向き(LocalTransform.quat の +Z)で代用する
	//--------------------------------------------------------------------------
	Math::Vector3 GetForward(Engine::ECS::World& a_world, Engine::ECS::Entity a_entity)
	{
		if (a_world.HasComponent<LookAngleComponent>(a_entity))
		{
			if (const auto* _pLook = a_world.RefData<LookAngleComponent>(a_entity))
			{
				return MakeLookForward(*_pLook);
			}
		}

		if (a_world.HasComponent<LocalTransformComponent>(a_entity))
		{
			if (const auto* _pTrs = a_world.RefData<LocalTransformComponent>(a_entity))
			{
				return Math::Vector3::Transform(Math::Vector3::Forward(), _pTrs->quat);
			}
		}

		return Math::Vector3::Forward();
	}
}

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

				const Math::Vector3 _toPre = _prePos - _selfPos;
				const float _distSq = _toPre.LengthSquared();
				const float _followDistance = _platoon.distance;
				const float _followDistanceSq = _followDistance * _followDistance;

				// すでに十分近いのなら停止
				if (_distSq <= _followDistanceSq)
				{
					_vel.value = Math::Vector3::Zero();
					continue;
				}

				const float _dist = std::sqrt(_distSq);

				const Math::Vector3 _direction = _toPre / _dist;

				//----------------------------------------------------------
				// 前の相手の後ろを目標地点にする
				//----------------------------------------------------------
				// 前方が分かるので「相手の真後ろ」を狙える。相手が曲がれば目標地点も
				// 一緒に回り込むので、列は相手の軌跡をなぞって付いていく
				//----------------------------------------------------------
				const Math::Vector3 _preForward = GetForward(_world, _pre);
				const Math::Vector3 _goalPos = _prePos - _preForward * _followDistance;

				// 前の相手の速度をベースに追従
				Math::Vector3 _target = _preVel;

				// 目標地点との位置誤差を補正
				_target += (_goalPos - _selfPos) * _platoon.followGain;

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
