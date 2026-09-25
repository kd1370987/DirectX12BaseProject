#include "SwarmLookSystem.h"

#include "Application/ECS/World/APPWorld.h"

#include "../../../../Components/Character/LookAngleComponent.h"
#include "../../../../Components/Character/BoidComponent.h"
#include "../../../../Components/Character/Boss/BoidLeaderComponent.h"
#include "../../../../Components/Character/Boss/PlatoonLeaderComponent.h"
#include "../../../../Components/Force/MovementComponent.h"

//==============================================================================
// SwarmLookSystem
//
// 群れのボス(リーダー / 小隊長 / ボイド)の「どちらを向いているか」を進める。
// 持ち物は既存の LookAngleComponent で、体の向き(quat)にするのは RotationSystem。
//
//   リーダー・小隊長 … 実際に進んでいる向き(MovementComponent.velocity)へ寄せる
//   ボイド           … 所属している小隊長(BoidComponent.platoonID)の向きへ寄せる
//
// ・寄せる速さは各コンポーネントの turnSpeedDeg(度/秒)。Yaw と Pitch に同じ値を使う。
// ・止まっている間(速度がほぼ 0)は向きを変えない。0 ベクトルから角度を作ると
//   どこを向くか決まらず、その場でくるくる回ってしまうため。
// ・小隊長はこの向きを「前の相手の後ろ」を求めるのに使う(PlatoonFollowSystem)。
//   つまり列の進路はリーダーの向きから順に伝わっていく。
// ・相手の角度は RefData で引く。クエリに入れると、同じ LookAngle を読む側と書く側で
//   依存が輪になるため。前フレームの値を見ることがあるが、1フレームの遅れで済む。
//==============================================================================
namespace
{
	//--------------------------------------------------------------------------
	// -180〜180 度に畳む
	//--------------------------------------------------------------------------
	float WrapDeg180(float a_deg)
	{
		a_deg = std::fmod(a_deg + 180.0f, 360.0f);
		if (a_deg < 0.0f) a_deg += 360.0f;
		return a_deg - 180.0f;
	}

	//--------------------------------------------------------------------------
	// 角度を目標へ最大 a_maxDelta だけ近づける(近い方の回り方で回る)
	//--------------------------------------------------------------------------
	float MoveTowardDeg(float a_current, float a_target, float a_maxDelta)
	{
		const float _diff = WrapDeg180(a_target - a_current);

		// 速さの指定が無いなら一気に合わせる
		if (a_maxDelta <= 0.0f) return WrapDeg180(a_target);

		if (std::fabs(_diff) <= a_maxDelta) return WrapDeg180(a_target);

		return WrapDeg180(a_current + std::copysign(a_maxDelta, _diff));
	}

	//--------------------------------------------------------------------------
	// 進んでいる向きへ視点角を寄せる
	//--------------------------------------------------------------------------
	void TurnToVelocity(
		LookAngleComponent& a_look,
		const Math::Vector3& a_velocity,
		float a_turnSpeedDeg,
		float a_dt)
	{
		float _yaw = 0.0f;
		float _pitch = 0.0f;
		if (!MakeLookAngleFromDir(a_velocity, _yaw, _pitch)) return;	// 止まっている間はそのまま

		const float _step = a_turnSpeedDeg * a_dt;

		a_look.Yaw = MoveTowardDeg(a_look.Yaw, _yaw, _step);
		a_look.Pitch = MoveTowardDeg(a_look.Pitch, _pitch, _step);
		a_look.Pitch = std::clamp(a_look.Pitch, -a_look.maxPitch, a_look.maxPitch);
	}
}

void SwarmLookSystem::Init(App::ECS::APPWorld& a_world)
{
	//--------------------------------------------------------------------------
	// リーダー : 進んでいる向きへ
	//--------------------------------------------------------------------------
	a_world.ActiveJobTask<const BoidLeaderComponent, const MovementComponent, LookAngleComponent>(
		Engine::ECS::ESystemType::Update,
		"SwarmLookSystem_Leader",
		[](
			Engine::ECS::Chunk*      a_pChunk,
			uint32_t                          a_count,
			const Engine::ECS::SystemContext& a_ctx,
			ActiveTag*                        a_tags,
			const BoidLeaderComponent*        a_leaderArray,
			const MovementComponent*          a_movementArray,
			LookAngleComponent*               a_lookArray
		)
		{
			for (size_t _i = 0; _i < a_count; ++_i)
			{
				TurnToVelocity(
					a_lookArray[_i], a_movementArray[_i].velocity,
					a_leaderArray[_i].turnSpeedDeg, a_ctx.dt);
			}
		}
	);

	//--------------------------------------------------------------------------
	// 小隊長 : 進んでいる向きへ
	//--------------------------------------------------------------------------
	a_world.ActiveTask<const PlatoonLeaderComponent, const MovementComponent, LookAngleComponent>(
		Engine::ECS::ESystemType::Update,
		"SwarmLookSystem_Platoon",
		[](
			Engine::ECS::Chunk*      a_pChunk,
			uint32_t                          a_count,
			const Engine::ECS::SystemContext& a_ctx,
			ActiveTag*                        a_tags,
			const PlatoonLeaderComponent*     a_platoonArray,
			const MovementComponent*          a_movementArray,
			LookAngleComponent*               a_lookArray
		)
		{
			for (size_t _i = 0; _i < a_count; ++_i)
			{
				TurnToVelocity(
					a_lookArray[_i], a_movementArray[_i].velocity,
					a_platoonArray[_i].turnSpeedDeg, a_ctx.dt);
			}
		}
	);

	//--------------------------------------------------------------------------
	// ボイド : 所属している小隊長の向きへ
	//--------------------------------------------------------------------------
	a_world.ActiveTask<const BoidComponent, LookAngleComponent>(
		Engine::ECS::ESystemType::Update,
		"SwarmLookSystem_Boid",
		[](
			Engine::ECS::Chunk*      a_pChunk,
			uint32_t                          a_count,
			const Engine::ECS::SystemContext& a_ctx,
			ActiveTag*                        a_tags,
			const BoidComponent*              a_boidArray,
			LookAngleComponent*               a_lookArray
		)
		{
			auto& _world = *a_ctx.pWorld;

			for (size_t _i = 0; _i < a_count; ++_i)
			{
				const BoidComponent& _boid = a_boidArray[_i];
				LookAngleComponent&  _look = a_lookArray[_i];

				// 所属が無い / 小隊長が居なくなったなら今の向きのまま
				const Engine::ECS::Entity _platoon = _boid.platoonID;
				if (_platoon == Engine::ECS::Limits::INVALID_ENTITY) continue;
				if (!_world.IsAliveEntity(_platoon)) continue;
				if (!_world.HasComponent<LookAngleComponent>(_platoon)) continue;

				const LookAngleComponent* _pTargetLook = _world.RefData<LookAngleComponent>(_platoon);
				if (!_pTargetLook) continue;

				const float _step = _boid.turnSpeedDeg * a_ctx.dt;

				_look.Yaw   = MoveTowardDeg(_look.Yaw, _pTargetLook->Yaw, _step);
				_look.Pitch = MoveTowardDeg(_look.Pitch, _pTargetLook->Pitch, _step);
				_look.Pitch = std::clamp(_look.Pitch, -_look.maxPitch, _look.maxPitch);
			}
		}
	);
}
