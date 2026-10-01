#include "Reorganize.h"

// エンジン
#include "Engine/GameObject/BaseObject/BaseObject.h"
#include "Engine/Persistence/Archive/Archive.h"
#include "Engine/ECS/System/SystemContext.h"
#include "Engine/Editor/Helper/EditorField.h"	// コンポーネントの Traits が使うので先に置く
#include "Engine/Graphics/DebugDraw/DebugDraw.h"
#include "Engine/Common/Color.h"

// App
#include "../../../../../ECS/World/APPWorld.h"
#include "../StateMachine.h"

#include "Application/Components/Transform/LocalTransformComponent.h"
#include "Application/Components/Movement/MoveIntentComponent.h"
#include "Application/Components/Movement/MovementParamsComponent.h"
#include "Application/Components/Movement/ActualVelocityComponent.h"
#include "Application/Components/Boid/SerchGroundComponent.h"

namespace App::Object
{
	namespace
	{
		// デバッグ表示で円を何本の線分で描くか
		constexpr int BALL_DRAW_SEGMENTS = 32;
	}

	void SwarmBossReorganizeState::Enter(SwarmBossStateContext& a_context)
	{
		ChangePhase(EPhase::Gather);
		m_time        = 0.0f;
		m_azimuth     = 0.0f;
		m_isRequested = false;

		if (!a_context.pObject || !a_context.pObject->pWorld) return;
		auto& _world = *a_context.pObject->pWorld;

		const auto _leader = a_context.leaderEntity;
		if (!_world.IsAliveEntity(_leader)) return;
		if (!_world.HasComponent<LocalTransformComponent>(_leader)) return;

		const Math::Vector3 _pos = _world.RefData<LocalTransformComponent>(_leader)->pos;

		//------------------------------------------------------------------
		// 球の中心 : 今の位置が赤道の上にあり、今の進行方向(水平)が接線になるところ
		//
		// 方位角が増える向きの接線は (-sin, 0, cos)。これを進行方向 v に合わせると、
		// 中心から見た今の位置の向き(外向き)は (v.z, 0, -v.x) になる
		//------------------------------------------------------------------
		Math::Vector3 _out = Math::Vector3(1.0f, 0.0f, 0.0f);
		if (_world.HasComponent<ActualVelocityComponent>(_leader))
		{
			Math::Vector3 _vel = _world.RefData<ActualVelocityComponent>(_leader)->value;
			_vel.y = 0.0f;
			if (_vel.LengthSquared() > 1e-4f)
			{
				_vel.Normalize();
				_out = Math::Vector3(_vel.z, 0.0f, -_vel.x);
			}
		}

		const float _radius = std::max(m_ballRadius, 0.0f);
		m_azimuth = std::atan2(_out.z, _out.x);
		m_center  = _pos - _out * _radius;

		// 地面に埋まらないように持ち上げる(地中に居たら飛び出してから巻く)
		if (_world.HasComponent<SerchGroundComponent>(_leader))
		{
			const SerchGroundComponent& _ground = *_world.RefData<SerchGroundComponent>(_leader);
			if (_ground.isFoundGround)
			{
				m_center.y = std::max(m_center.y, _ground.groundHeight + _radius + m_clearance);
			}
		}
	}

	void SwarmBossReorganizeState::Update(SwarmBossStateContext& a_context)
	{
		if (m_phase == EPhase::End) return;
		if (!a_context.pObject || !a_context.pObject->pWorld) return;
		auto& _world = *a_context.pObject->pWorld;
		const float _dt = a_context.pObject->dt;

		// 整理している間は体を無敵にする(抜ければ依頼の既定値 1 に戻る)
		a_context.bodyDefenseRatio = m_defenseRatio;

		const auto _leader = a_context.leaderEntity;
		if (!_world.IsAliveEntity(_leader)) return;
		if (!_world.HasComponent<MoveIntentComponent>(_leader)) return;
		if (!_world.HasComponent<LocalTransformComponent>(_leader)) return;

		const Math::Vector3 _pos = _world.RefData<LocalTransformComponent>(_leader)->pos;

		// 入力 1 で出る速さ。球の上の目標点をリーダーの速さで進めるのに使う
		const float _moveSpeed = _world.HasComponent<MovementParamsComponent>(_leader)
			? _world.RefData<MovementParamsComponent>(_leader)->moveSpeed
			: 0.0f;

		m_time      += _dt;
		m_phaseTime += _dt;

		//--------------------------------------------------------------
		// 球の表面を進む目標点を追う(どのフェーズでも回り続ける)
		//
		//   入力 = 目標点の速度 / moveSpeed  +  目標点とのずれ × 詰める強さ / moveSpeed
		//
		// 方位角は水平の速さがリーダーの速さになるように進める(極に近いほど速く回る)
		//--------------------------------------------------------------
		const float _radius = std::max(m_ballRadius, 0.0f);
		const float _speed  = _moveSpeed * std::max(m_speedScale, 0.0f);

		float _azimuthSpeed = 0.0f;
		if (_radius > 1e-3f)
		{
			// 極の近くで割り過ぎないよう、半径の下限を入れる
			const float _ringRadius = std::max(_radius * std::sin(CalcPolar(m_time)), _radius * 0.2f);
			_azimuthSpeed = _speed / _ringRadius;
		}
		m_azimuth += _azimuthSpeed * _dt;
		m_azimuth  = std::remainder(m_azimuth, DirectX::XM_2PI);	// 長く回っても精度が落ちないように畳む

		Math::Vector3 _intent = Math::Vector3(0.0f, 0.0f, 0.0f);
		if (_moveSpeed > 0.0f)
		{
			_intent = CalcTargetVelocity(m_time, _azimuthSpeed) / _moveSpeed
				+ (CalcTargetPos(m_time) - _pos) * (m_followGain / _moveSpeed);
		}
		_world.RefData<MoveIntentComponent>(_leader)->value = _intent;

		DrawBall(a_context);

		switch (m_phase)
		{
		//--------------------------------------------------------------
		// まとまる : 時間が来たら小隊長の整理を頼む(頼むのは一度だけ)
		//--------------------------------------------------------------
		case EPhase::Gather:
		{
			if (m_phaseTime >= m_gatherTime)
			{
				a_context.isRequestReorganize = true;
				m_isRequested = true;
				ChangePhase(EPhase::Settle);
			}
			break;
		}

		//--------------------------------------------------------------
		// 落ち着く : 割り当て直したボイドが新しい小隊長へ寄るのを待ってから徘徊へ
		//--------------------------------------------------------------
		case EPhase::Settle:
		{
			if (m_phaseTime >= m_settleTime)
			{
				Finish(a_context);
			}
			break;
		}

		default:
			break;
		}
	}

	void SwarmBossReorganizeState::Exit(SwarmBossStateContext& a_context)
	{
		// 回る速さの入力を次のステートへ持ち越さない
		if (!a_context.pObject || !a_context.pObject->pWorld) return;
		auto& _world = *a_context.pObject->pWorld;

		const auto _leader = a_context.leaderEntity;
		if (!_world.IsAliveEntity(_leader)) return;
		if (!_world.HasComponent<MoveIntentComponent>(_leader)) return;

		_world.RefData<MoveIntentComponent>(_leader)->value = Math::Vector3(0.0f, 0.0f, 0.0f);
	}

	void SwarmBossReorganizeState::ChangePhase(EPhase a_phase)
	{
		m_phase     = a_phase;
		m_phaseTime = 0.0f;
	}

	Math::Vector3 SwarmBossReorganizeState::CalcTargetPos(float a_time) const
	{
		// 極角 θ(真上から)と方位角 φ の球面座標
		const float _polar  = CalcPolar(a_time);
		const float _radius = std::max(m_ballRadius, 0.0f);

		return m_center + Math::Vector3(
			std::sin(_polar) * std::cos(m_azimuth),
			std::cos(_polar),
			std::sin(_polar) * std::sin(m_azimuth)) * _radius;
	}

	Math::Vector3 SwarmBossReorganizeState::CalcTargetVelocity(float a_time, float a_azimuthSpeed) const
	{
		// CalcTargetPos の時間微分(θ と φ の両方が動く)
		const float _polar      = CalcPolar(a_time);
		const float _polarSpeed = CalcPolarSpeed(a_time);
		const float _radius     = std::max(m_ballRadius, 0.0f);

		const float _sinT = std::sin(_polar);
		const float _cosT = std::cos(_polar);
		const float _sinP = std::sin(m_azimuth);
		const float _cosP = std::cos(m_azimuth);

		return Math::Vector3(
			_cosT * _cosP * _polarSpeed - _sinT * _sinP * a_azimuthSpeed,
			-_sinT * _polarSpeed,
			_cosT * _sinP * _polarSpeed + _sinT * _cosP * a_azimuthSpeed) * _radius;
	}

	float SwarmBossReorganizeState::CalcPolar(float a_time) const
	{
		// 赤道(π/2)から上下へ振れる。入ったときは赤道なので、巻き始めで跳ねない
		const float _amp = DirectX::XMConvertToRadians(std::clamp(m_polarAmplitudeDeg, 0.0f, 89.0f));
		if (m_polarPeriod <= 0.0f) return DirectX::XM_PIDIV2;

		return DirectX::XM_PIDIV2 + _amp * std::sin(DirectX::XM_2PI * a_time / m_polarPeriod);
	}

	float SwarmBossReorganizeState::CalcPolarSpeed(float a_time) const
	{
		// CalcPolar の時間微分
		if (m_polarPeriod <= 0.0f) return 0.0f;

		const float _amp   = DirectX::XMConvertToRadians(std::clamp(m_polarAmplitudeDeg, 0.0f, 89.0f));
		const float _omega = DirectX::XM_2PI / m_polarPeriod;
		return _amp * _omega * std::cos(_omega * a_time);
	}

	void SwarmBossReorganizeState::DrawBall(SwarmBossStateContext& a_context) const
	{
		if (!a_context.pObject->pServices || !a_context.pObject->pServices->pDebugDraw) return;
		auto& _debugDraw = *a_context.pObject->pServices->pDebugDraw;

		const float _radius = std::max(m_ballRadius, 0.0f);

		// 赤道(水平)と子午線(縦。XY 面)
		Math::Vector3 _prevH = m_center + Math::Vector3(_radius, 0.0f, 0.0f);
		Math::Vector3 _prevV = _prevH;
		for (int _i = 1; _i <= BALL_DRAW_SEGMENTS; ++_i)
		{
			const float _a = DirectX::XM_2PI * static_cast<float>(_i) / BALL_DRAW_SEGMENTS;
			const Math::Vector3 _nextH = m_center + Math::Vector3(std::cos(_a), 0.0f, std::sin(_a)) * _radius;
			const Math::Vector3 _nextV = m_center + Math::Vector3(std::cos(_a), std::sin(_a), 0.0f) * _radius;
			_debugDraw.DrawLine(_prevH, _nextH, Engine::Color::BLUE);
			_debugDraw.DrawLine(_prevV, _nextV, Engine::Color::BLUE);
			_prevH = _nextH;
			_prevV = _nextV;
		}
	}

	void SwarmBossReorganizeState::Finish(SwarmBossStateContext& a_context)
	{
		if (m_phase == EPhase::End) return;
		m_phase = EPhase::End;

		if (a_context.pMachine)
		{
			a_context.pMachine->RequestChangeState(ESwarmBossState::RandomWalk);
		}
	}

	void SwarmBossReorganizeState::Archive(Engine::Persistence::Archive& a_ar)
	{
		// ステートの調整値は同じ階層に並ぶので、名前の頭に Reorganize を付けて区別する
		a_ar.Field("ReorganizeBallRadius", m_ballRadius);
		a_ar.Field("ReorganizeClearance", m_clearance);
		a_ar.Field("ReorganizeSpeedScale", m_speedScale);
		a_ar.Field("ReorganizeFollowGain", m_followGain);
		a_ar.Field("ReorganizePolarAmplitudeDeg", m_polarAmplitudeDeg);
		a_ar.Field("ReorganizePolarPeriod", m_polarPeriod);
		a_ar.Field("ReorganizeGatherTime", m_gatherTime);
		a_ar.Field("ReorganizeSettleTime", m_settleTime);
		a_ar.Field("ReorganizeDefenseRatio", m_defenseRatio);
	}

	void SwarmBossReorganizeState::DrawInspector()
	{
		Engine::Editor::Field("Ball Radius", m_ballRadius, 0.5f, 0.0f);
		Engine::Editor::Field("Clearance", m_clearance, 0.5f, 0.0f);
		Engine::Editor::Tooltip("Keep the bottom of the ball this high above the ground");
		Engine::Editor::Field("Speed Scale", m_speedScale, 0.05f, 0.0f);
		Engine::Editor::Tooltip("Speed scale above Platoon Scale tears the line apart");
		Engine::Editor::Field("Follow Gain", m_followGain, 0.05f, 0.0f);
		Engine::Editor::Field("Polar Amplitude", m_polarAmplitudeDeg, 1.0f, 0.0f, 89.0f);
		Engine::Editor::Field("Polar Period", m_polarPeriod, 0.05f, 0.0f);
		Engine::Editor::Field("Gather Time", m_gatherTime, 0.05f, 0.0f);
		Engine::Editor::Field("Settle Time", m_settleTime, 0.05f, 0.0f);
		Engine::Editor::Field("Defense Ratio", m_defenseRatio, 0.01f, 0.0f, 1.0f);
		Engine::Editor::Tooltip("Body damage ratio while reorganizing (0 : invincible)");

		// 実行中の状態は表示のみ
		Engine::Editor::Value("Phase", "%s (%.1f s)", std::string(magic_enum::enum_name(m_phase)).c_str(), m_phaseTime);
		Engine::Editor::Value("Center", "%.1f, %.1f, %.1f", m_center.x, m_center.y, m_center.z);
		Engine::Editor::Value("Requested", "%s", m_isRequested ? "yes" : "no");
	}
}
