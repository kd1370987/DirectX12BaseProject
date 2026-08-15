#include "LookAroundSystem.h"

#include "Engine/ECS/World/World.h"

#include "../../../../Components/Character/TargetEntityComponent.h"
#include "../../../../Components/Character/PatrolComponent.h"
#include "../../../../Components/Transform/LocalTransformComponent.h"

//==============================================================================
// LookAroundSystem
//
// 戦闘モードに入っていない敵の旋回を担当する。
//
//   徘徊 Move       … 歩いている方向を向く
//   徘徊 Pause      … 立ち止まった向きを中心に、少し首を振る
//   見失い MoveTo    … 最後に見た地点の方向を向きながら移動する
//   見失い LookAround… 到着したときの向きを中心に、大きく首を振って辺りを見渡す
//
// ・戦闘モード中(isFind == true)は何もしない。そちらは FaceTargetSystem が
//   プレイヤーの方向へ旋回させる。条件が排他なので同じ quat を奪い合わない。
// ・旋回は Y 軸まわり(Yaw)のみ。左手系 +Z 前方なので Yaw = atan2(x, z)。
// ・首振りは sin 波そのものが滑らかな軌道なので目標角を直接入れる。
//   基準が「止まった瞬間の向き」なので切り替わった瞬間も飛ばない。
//   方向転換(Move / MoveTo)のほうは Slerp で追従させる。
// ・Update 帯に置く。書いた quat は PostUpdate の行列計算で反映される。
//==============================================================================
namespace
{
	// 基準角から左右へ首を振ったときの目標ヨー(ラジアン)
	//
	// offset(t) = A * sin(ωt) の角速度のピークが a_speedDeg になるよう
	// ω = speed / A とする(A = a_ampDeg)。
	float SweepYaw(float a_baseYaw, float a_ampDeg, float a_speedDeg, float a_elapsed)
	{
		if (a_ampDeg <= 1e-3f) return a_baseYaw;

		float _omega     = a_speedDeg / a_ampDeg;
		float _offsetDeg = a_ampDeg * std::sin(a_elapsed * _omega);

		return a_baseYaw + DirectX::XMConvertToRadians(_offsetDeg);
	}
}

void LookAroundSystem::Init(Engine::ECS::World& a_world)
{
	a_world.ActiveTask<const TargetEntityComponent, const PatrolComponent, LocalTransformComponent>(
		Engine::ECS::ESystemType::Update,
		"LookAroundSystem",
		[](
			Engine::ECS::ArchetypeChunk*      a_pChunk,
			uint32_t                          a_count,
			const Engine::ECS::SystemContext& a_ctx,
			ActiveTag*                        a_tags,
			const TargetEntityComponent*      a_targetArray,
			const PatrolComponent*            a_patrolArray,
			LocalTransformComponent*          a_trsArray
		)
		{
			// 方向転換の旋回速度(1秒あたりの補間強度。FaceTargetSystem に合わせた値)
			constexpr float _kTurnSpeed = 10.0f;

			for (size_t _i = 0; _i < a_count; ++_i)
			{
				const TargetEntityComponent& _target = a_targetArray[_i];
				const PatrolComponent&       _patrol = a_patrolArray[_i];
				LocalTransformComponent&     _trs    = a_trsArray[_i];

				// 視認中は FaceTargetSystem に任せる
				if (_target.isFind) continue;

				float _targetYaw = 0.0f;
				bool  _isSmooth  = true;	// 目標へ Slerp で追従するか(false は直接入れる)

				// 向きたい水平方向。決まったものだけ Yaw に変換する
				DXSM::Vector3 _dir    = {};
				bool          _hasDir = false;

				switch (_patrol.lostPhase)
				{
				case ELostPhase::MoveTo:
					// 向かっている地点の方向を向く
					_dir    = DXSM::Vector3(_patrol.lastSeenPos) - DXSM::Vector3(_trs.pos);
					_hasDir = true;
					break;

				case ELostPhase::LookAround:
					// 到着時の向きを中心に大きく見渡す
					_targetYaw = SweepYaw(
						_patrol.lookAroundYaw, _patrol.lookAroundYawDeg,
						_patrol.lookAroundSpeedDeg, _patrol.lostTimer);
					_isSmooth = false;
					break;

				default:
					// 徘徊中
					if (_patrol.patrolPhase == EPatrolPhase::Pause)
					{
						// 立ち止まった向きを中心に少し首を振る
						// wanderTimer は残り時間なので、経過時間へ直してから渡す
						float _elapsed = _patrol.patrolPauseTime - _patrol.wanderTimer;
						_targetYaw = SweepYaw(
							_patrol.patrolLookYaw, _patrol.patrolLookYawDeg,
							_patrol.lookAroundSpeedDeg, _elapsed);
						_isSmooth = false;
					}
					else
					{
						// 歩いている方向を向く
						_dir    = DXSM::Vector3(_patrol.wanderDir);
						_hasDir = true;
					}
					break;
				}

				if (_hasDir)
				{
					_dir.y = 0.0f;

					float _lenSq = _dir.LengthSquared();
					if (!(_lenSq > 1e-6f)) continue;	// ほぼ真上/同一座標は旋回不能
					_dir /= std::sqrt(_lenSq);

					_targetYaw = std::atan2(_dir.x, _dir.z);
				}

				DirectX::XMVECTOR _targetQuat =
					DirectX::XMQuaternionRotationRollPitchYaw(0.0f, _targetYaw, 0.0f);

				if (_isSmooth)
				{
					DirectX::XMVECTOR _currentQuat = DirectX::XMLoadFloat4(&_trs.quat);
					float _t = std::min(_kTurnSpeed * a_ctx.dt, 1.0f);
					_targetQuat = DirectX::XMQuaternionSlerp(_currentQuat, _targetQuat, _t);
				}

				DirectX::XMStoreFloat4(&_trs.quat, _targetQuat);
				_trs.isDirty = true;	// 停止中でも行列を再構築させる
			}
		}
	);
}
