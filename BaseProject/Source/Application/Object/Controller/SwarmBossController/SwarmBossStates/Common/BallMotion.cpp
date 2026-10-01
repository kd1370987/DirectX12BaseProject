#include "BallMotion.h"

// エンジン
#include "Engine/GameObject/BaseObject/BaseObject.h"
#include "Engine/Persistence/Archive/Archive.h"
#include "Engine/ECS/System/SystemContext.h"
#include "Engine/Editor/Helper/EditorField.h"	// コンポーネントの Traits が使うので先に置く
#include "Engine/Graphics/DebugDraw/DebugDraw.h"
#include "Engine/Common/Color.h"

// App
#include "../../../../../ECS/World/APPWorld.h"

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

	void SwarmBossBallMotion::Begin(SwarmBossStateContext& a_context, float a_centerHeight)
	{
		m_azimuth     = 0.0f;
		m_time        = 0.0f;
		m_radiusScale = 1.0f;

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

		//------------------------------------------------------------------
		// 高さ : 地表から決まった高さか、地面に埋まらないように持ち上げるだけか
		// (地面が見つからなければ今の高さのまま)
		//------------------------------------------------------------------
		if (_world.HasComponent<SerchGroundComponent>(_leader))
		{
			const SerchGroundComponent& _ground = *_world.RefData<SerchGroundComponent>(_leader);
			if (_ground.isFoundGround)
			{
				m_center.y = (a_centerHeight >= 0.0f)
					? _ground.groundHeight + a_centerHeight
					: std::max(m_center.y, _ground.groundHeight + _radius + m_clearance);
			}
		}
	}

	void SwarmBossBallMotion::Update(SwarmBossStateContext& a_context, float a_dt, float a_radiusScale)
	{
		if (!a_context.pObject || !a_context.pObject->pWorld) return;
		auto& _world = *a_context.pObject->pWorld;

		const auto _leader = a_context.leaderEntity;
		if (!_world.IsAliveEntity(_leader)) return;
		if (!_world.HasComponent<MoveIntentComponent>(_leader)) return;
		if (!_world.HasComponent<LocalTransformComponent>(_leader)) return;

		const Math::Vector3 _pos = _world.RefData<LocalTransformComponent>(_leader)->pos;

		// 入力 1 で出る速さ。球の上の目標点をリーダーの速さで進めるのに使う
		const float _moveSpeed = _world.HasComponent<MovementParamsComponent>(_leader)
			? _world.RefData<MovementParamsComponent>(_leader)->moveSpeed
			: 0.0f;

		m_time        += a_dt;
		m_radiusScale  = std::max(a_radiusScale, 0.0f);

		//--------------------------------------------------------------
		// 球の表面を進む目標点を追う
		//
		//   入力 = 目標点の速度 / moveSpeed  +  目標点とのずれ × 詰める強さ / moveSpeed
		//
		// 方位角は水平の速さがリーダーの速さになるように進める(極に近いほど速く回る)
		//--------------------------------------------------------------
		const float _radius = GetRadius();
		const float _speed  = _moveSpeed * std::max(m_speedScale, 0.0f);

		float _azimuthSpeed = 0.0f;
		if (_radius > 1e-3f)
		{
			// 極の近くで割り過ぎないよう、半径の下限を入れる
			const float _ringRadius = std::max(_radius * std::sin(CalcPolar()), _radius * 0.2f);
			_azimuthSpeed = _speed / _ringRadius;
		}
		m_azimuth += _azimuthSpeed * a_dt;
		m_azimuth  = std::remainder(m_azimuth, DirectX::XM_2PI);	// 長く回っても精度が落ちないように畳む

		Math::Vector3 _intent = Math::Vector3(0.0f, 0.0f, 0.0f);
		if (_moveSpeed > 0.0f)
		{
			_intent = CalcTargetVelocity(_azimuthSpeed) / _moveSpeed
				+ (CalcTargetPos() - _pos) * (m_followGain / _moveSpeed);
		}
		_world.RefData<MoveIntentComponent>(_leader)->value = _intent;
	}

	Math::Vector3 SwarmBossBallMotion::CalcTargetPos() const
	{
		// 極角 θ(真上から)と方位角 φ の球面座標
		const float _polar = CalcPolar();

		return m_center + Math::Vector3(
			std::sin(_polar) * std::cos(m_azimuth),
			std::cos(_polar),
			std::sin(_polar) * std::sin(m_azimuth)) * GetRadius();
	}

	Math::Vector3 SwarmBossBallMotion::CalcTargetVelocity(float a_azimuthSpeed) const
	{
		// CalcTargetPos の時間微分(θ と φ の両方が動く。半径の変化は小さいので含めない)
		const float _polar      = CalcPolar();
		const float _polarSpeed = CalcPolarSpeed();

		const float _sinT = std::sin(_polar);
		const float _cosT = std::cos(_polar);
		const float _sinP = std::sin(m_azimuth);
		const float _cosP = std::cos(m_azimuth);

		return Math::Vector3(
			_cosT * _cosP * _polarSpeed - _sinT * _sinP * a_azimuthSpeed,
			-_sinT * _polarSpeed,
			_cosT * _sinP * _polarSpeed + _sinT * _cosP * a_azimuthSpeed) * GetRadius();
	}

	float SwarmBossBallMotion::CalcPolar() const
	{
		// 赤道(π/2)から上下へ振れる。入ったときは赤道なので、巻き始めで跳ねない
		const float _amp = DirectX::XMConvertToRadians(std::clamp(m_polarAmplitudeDeg, 0.0f, 89.0f));
		if (m_polarPeriod <= 0.0f) return DirectX::XM_PIDIV2;

		return DirectX::XM_PIDIV2 + _amp * std::sin(DirectX::XM_2PI * m_time / m_polarPeriod);
	}

	float SwarmBossBallMotion::CalcPolarSpeed() const
	{
		// CalcPolar の時間微分
		if (m_polarPeriod <= 0.0f) return 0.0f;

		const float _amp   = DirectX::XMConvertToRadians(std::clamp(m_polarAmplitudeDeg, 0.0f, 89.0f));
		const float _omega = DirectX::XM_2PI / m_polarPeriod;
		return _amp * _omega * std::cos(_omega * m_time);
	}

	void SwarmBossBallMotion::Draw(SwarmBossStateContext& a_context) const
	{
		if (!a_context.pObject || !a_context.pObject->pServices || !a_context.pObject->pServices->pDebugDraw) return;
		auto& _debugDraw = *a_context.pObject->pServices->pDebugDraw;

		const float _radius = GetRadius();

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

	void SwarmBossBallMotion::Archive(Engine::Persistence::Archive& a_ar, const std::string& a_prefix)
	{
		a_ar.Field(a_prefix + "BallRadius", m_ballRadius);
		a_ar.Field(a_prefix + "Clearance", m_clearance);
		a_ar.Field(a_prefix + "SpeedScale", m_speedScale);
		a_ar.Field(a_prefix + "FollowGain", m_followGain);
		a_ar.Field(a_prefix + "PolarAmplitudeDeg", m_polarAmplitudeDeg);
		a_ar.Field(a_prefix + "PolarPeriod", m_polarPeriod);
	}

	void SwarmBossBallMotion::DrawInspector()
	{
		Engine::Editor::Field("Ball Radius", m_ballRadius, 0.5f, 0.0f);
		Engine::Editor::Field("Clearance", m_clearance, 0.5f, 0.0f);
		Engine::Editor::Tooltip("Keep the bottom of the ball this high above the ground (when height is not fixed)");
		Engine::Editor::Field("Speed Scale", m_speedScale, 0.05f, 0.0f);
		Engine::Editor::Tooltip("Speed scale above Platoon Scale tears the line apart");
		Engine::Editor::Field("Follow Gain", m_followGain, 0.05f, 0.0f);
		Engine::Editor::Field("Polar Amplitude", m_polarAmplitudeDeg, 1.0f, 0.0f, 89.0f);
		Engine::Editor::Field("Polar Period", m_polarPeriod, 0.05f, 0.0f);

		// 実行中の状態は表示のみ
		Engine::Editor::Value("Center", "%.1f, %.1f, %.1f (r %.1f)", m_center.x, m_center.y, m_center.z, GetRadius());
	}
}
