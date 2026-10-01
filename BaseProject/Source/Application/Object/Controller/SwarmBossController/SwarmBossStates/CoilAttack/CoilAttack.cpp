#include "CoilAttack.h"

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
#include "Application/Components/Transform/WorldMatrixComponent.h"
#include "Application/Components/Movement/MoveIntentComponent.h"
#include "Application/Components/Movement/MovementParamsComponent.h"
#include "Application/Components/Movement/ActualVelocityComponent.h"
#include "Application/Components/Input/PlayerControllTag.h"
#include "Application/Components/Boid/SerchGroundComponent.h"
#include "Application/InstanceResource/SwarmMissileResource.h"

namespace App::Object
{
	namespace
	{
		//----------------------------------------------------------------------
		// 操作しているプレイヤーの位置(ワールド)。見つからなければ false
		//
		// プレイヤーは親を持つとは限らないので、行列から位置を取る
		//----------------------------------------------------------------------
		bool FindPlayerPos(Engine::GameObject::ObjectContext& a_context, Math::Vector3& a_outPos)
		{
			Engine::ECS::Entity _player = Engine::ECS::Limits::INVALID_ENTITY;

			a_context.pWorld->ForEach<const ActiveTag, const PlayerControllTag>(
				[&](
					Engine::ECS::Chunk* a_pChunk,
					uint32_t a_count,
					const ActiveTag* a_activeTagArray,
					const PlayerControllTag* a_playerTagArray
				)
				{
					if (_player != Engine::ECS::Limits::INVALID_ENTITY || a_count == 0) return;
					_player = a_pChunk->entityData[0];
				}
			);

			if (_player == Engine::ECS::Limits::INVALID_ENTITY) return false;

			auto& _world = *a_context.pWorld;
			if (_world.HasComponent<WorldMatrixComponent>(_player))
			{
				a_outPos = _world.RefData<WorldMatrixComponent>(_player)->worldMat.Translation();
				return true;
			}
			if (_world.HasComponent<LocalTransformComponent>(_player))
			{
				a_outPos = _world.RefData<LocalTransformComponent>(_player)->pos;
				return true;
			}
			return false;
		}

		// 正規化して長さを掛ける。長さ0なら0を返す
		Math::Vector3 ScaledDir(Math::Vector3 a_dir, float a_scale)
		{
			if (a_dir.LengthSquared() <= 1e-6f) return Math::Vector3(0.0f, 0.0f, 0.0f);
			a_dir.Normalize();
			return a_dir * a_scale;
		}

		// デバッグ表示で輪を何本の線分で描くか
		constexpr int RING_DRAW_SEGMENTS = 48;
	}

	void SwarmBossCoilAttackState::Enter(SwarmBossStateContext& a_context)
	{
		ChangePhase(EPhase::Approach);

		m_angle         = 0.0f;
		m_turnSign      = 1.0f;
		m_isGroundKnown = false;
		m_launchTimer   = 0.0f;
		m_launchCursor  = 0;
		m_launchCount   = 0;

		// 中心は最初に見つけたプレイヤーの位置から始める(見つからなければ Update で抜ける)
		if (a_context.pObject && a_context.pObject->pWorld && FindPlayerPos(*a_context.pObject, m_playerPos))
		{
			m_center = m_playerPos;
		}
	}

	void SwarmBossCoilAttackState::Update(SwarmBossStateContext& a_context)
	{
		if (m_phase == EPhase::End) return;
		if (!a_context.pObject || !a_context.pObject->pWorld) return;
		auto& _world = *a_context.pObject->pWorld;
		const float _dt = a_context.pObject->dt;

		const auto _leader = a_context.leaderEntity;
		if (!_world.IsAliveEntity(_leader)) return;
		if (!_world.HasComponent<MoveIntentComponent>(_leader)) return;
		if (!_world.HasComponent<LocalTransformComponent>(_leader)) return;

		// プレイヤーが居ない(倒された / まだ湧いていない)なら巻く相手が居ない
		if (!FindPlayerPos(*a_context.pObject, m_playerPos))
		{
			Finish(a_context);
			return;
		}

		const Math::Vector3 _pos = _world.RefData<LocalTransformComponent>(_leader)->pos;

		// 入力 1 で出る速さ。輪の上の目標点をリーダーの速さで進めるのに使う
		const float _moveSpeed = _world.HasComponent<MovementParamsComponent>(_leader)
			? _world.RefData<MovementParamsComponent>(_leader)->moveSpeed
			: 0.0f;

		// 地表の高さ(SerchGroundSystem が書いた結果)。
		// レイが届かない高さに居るフレームは最後に見た値のまま。一度も見ていなければプレイヤーの足元
		if (_world.HasComponent<SerchGroundComponent>(_leader))
		{
			const SerchGroundComponent& _ground = *_world.RefData<SerchGroundComponent>(_leader);
			if (_ground.isFoundGround)
			{
				m_groundHeight  = _ground.groundHeight;
				m_isGroundKnown = true;
			}
		}
		if (!m_isGroundKnown) m_groundHeight = m_playerPos.y;

		m_radius = CalcRadius(a_context.wormLength);
		m_phaseTime += _dt;

		Math::Vector3 _intent = Math::Vector3(0.0f, 0.0f, 0.0f);

		switch (m_phase)
		{
		//--------------------------------------------------------------
		// 輪に入る : 輪の上で一番近い点(中心から見て今の自分側)へ向かう。
		// 輪に入るまでは中心をプレイヤーにぴったり付ける
		//--------------------------------------------------------------
		case EPhase::Approach:
		{
			m_center = m_playerPos;

			Math::Vector3 _out = _pos - m_center;
			_out.y = 0.0f;
			if (_out.LengthSquared() <= 1e-6f) _out = Math::Vector3(1.0f, 0.0f, 0.0f);
			_out.Normalize();

			Math::Vector3 _entry = m_center + _out * m_radius;
			_entry.y = m_groundHeight + CalcHeight(0.0f);

			const Math::Vector3 _toEntry = _entry - _pos;
			_intent = ScaledDir(_toEntry, std::max(m_approachSpeedScale, 0.0f));

			if (_toEntry.Length() <= m_arriveDistance || m_phaseTime >= m_approachMaxTime)
			{
				// 輪の上の、今の自分に一番近い角度から回り始める
				m_angle = std::atan2(_out.z, _out.x);

				// 回る向きは今進んでいる向きに近い方(急に折り返さないように)
				m_turnSign = (Math::Random::Float(0.0f, 1.0f) < 0.5f) ? 1.0f : -1.0f;
				if (_world.HasComponent<ActualVelocityComponent>(_leader))
				{
					const Math::Vector3 _vel = _world.RefData<ActualVelocityComponent>(_leader)->value;
					const Math::Vector3 _tangent(-std::sin(m_angle), 0.0f, std::cos(m_angle));
					const float _side = _vel.Dot(_tangent);
					if (std::fabs(_side) > 1e-3f) m_turnSign = (_side >= 0.0f) ? 1.0f : -1.0f;
				}

				// 巻き始めてから切り離しを始めるまでの時間。以降は間隔ごと
				m_launchTimer = std::max(m_launchStartDelay, 0.0f);

				ChangePhase(EPhase::Coil);
			}
			break;
		}

		//--------------------------------------------------------------
		// 巻く : 輪の上を進む目標点を追う
		//
		//   入力 = 目標点の速度 / moveSpeed  +  目標点とのずれ × 詰める強さ / moveSpeed
		//
		// 目標点は水平にはリーダーの速さで輪を回り、高さは正弦波で地面と上を行き来する
		//--------------------------------------------------------------
		case EPhase::Coil:
		{
			// 中心をプレイヤーへゆっくり寄せる(高さは使わない)
			const float _follow = std::clamp(m_centerFollowGain * _dt, 0.0f, 1.0f);
			m_center.x += (m_playerPos.x - m_center.x) * _follow;
			m_center.z += (m_playerPos.z - m_center.z) * _follow;

			const float _speed = _moveSpeed * std::max(m_coilSpeedScale, 0.0f);
			if (m_radius > 1e-3f)
			{
				m_angle += m_turnSign * (_speed / m_radius) * _dt;
				m_angle  = std::remainder(m_angle, DirectX::XM_2PI);	// 長く回っても精度が落ちないように畳む
			}

			const float _cos = std::cos(m_angle);
			const float _sin = std::sin(m_angle);

			Math::Vector3 _target = m_center + Math::Vector3(_cos, 0.0f, _sin) * m_radius;
			_target.y = m_groundHeight + CalcHeight(m_phaseTime);

			const Math::Vector3 _targetVel =
				Math::Vector3(-_sin, 0.0f, _cos) * (m_turnSign * _speed) +
				Math::Vector3(0.0f, CalcHeightSpeed(m_phaseTime), 0.0f);

			if (_moveSpeed > 0.0f)
			{
				_intent = _targetVel / _moveSpeed + (_target - _pos) * (m_followGain / _moveSpeed);
			}

			// 頭の小隊長から尾へ順番に、1体ずつ切り離す
			m_launchTimer -= _dt;
			while (m_launchTimer <= 0.0f)
			{
				RequestLaunch(a_context);
				m_launchTimer += std::max(m_launchInterval, 0.01f);	// 0だと止まらないので下限を入れる
			}

			if (m_phaseTime >= m_coilTime)
			{
				Finish(a_context);
				return;
			}
			break;
		}

		default:
			break;
		}

		_world.RefData<MoveIntentComponent>(_leader)->value = _intent;

		WriteMissileResource(a_context);
		DrawRing(a_context);
	}

	void SwarmBossCoilAttackState::Exit(SwarmBossStateContext& a_context)
	{
		// 巻く速さの入力を次のステートへ持ち越さない
		if (!a_context.pObject || !a_context.pObject->pWorld) return;
		auto& _world = *a_context.pObject->pWorld;

		const auto _leader = a_context.leaderEntity;
		if (!_world.IsAliveEntity(_leader)) return;
		if (!_world.HasComponent<MoveIntentComponent>(_leader)) return;

		_world.RefData<MoveIntentComponent>(_leader)->value = Math::Vector3(0.0f, 0.0f, 0.0f);
	}

	void SwarmBossCoilAttackState::ChangePhase(EPhase a_phase)
	{
		m_phase     = a_phase;
		m_phaseTime = 0.0f;
	}

	float SwarmBossCoilAttackState::CalcRadius(float a_wormLength) const
	{
		// 円周 = 体の長さ なら頭と尾がちょうど繋がる
		const float _radius = std::max(a_wormLength, 0.0f) / DirectX::XM_2PI * std::max(m_radiusScale, 0.0f);
		return std::max(_radius, m_minRadius);
	}

	float SwarmBossCoilAttackState::CalcHeight(float a_time) const
	{
		// 真ん中から上がり始める(輪に入るときの高さが真ん中なので、巻き始めで跳ねない)
		const float _mid = (m_highHeight + m_lowHeight) * 0.5f;
		const float _amp = (m_highHeight - m_lowHeight) * 0.5f;
		if (m_undulationPeriod <= 0.0f) return _mid;

		return _mid + _amp * std::sin(DirectX::XM_2PI * a_time / m_undulationPeriod);
	}

	float SwarmBossCoilAttackState::CalcHeightSpeed(float a_time) const
	{
		// CalcHeight の時間微分
		if (m_undulationPeriod <= 0.0f) return 0.0f;

		const float _amp   = (m_highHeight - m_lowHeight) * 0.5f;
		const float _omega = DirectX::XM_2PI / m_undulationPeriod;
		return _amp * _omega * std::cos(_omega * a_time);
	}

	void SwarmBossCoilAttackState::RequestLaunch(SwarmBossStateContext& a_context)
	{
		if (!a_context.pPlatoonLeaders || a_context.pPlatoonLeaders->empty()) return;
		auto& _world = *a_context.pObject->pWorld;
		if (!_world.HasResource<SwarmMissileResource>()) return;

		const auto& _platoons = *a_context.pPlatoonLeaders;
		const size_t _count = _platoons.size();

		// 居なくなった小隊長は飛ばして、次に居る小隊長へ。一周しても居なければ何もしない
		for (size_t _attempt = 0; _attempt < _count; ++_attempt)
		{
			const size_t _index = m_launchCursor % _count;
			m_launchCursor = (_index + 1) % _count;

			const Engine::ECS::Entity _platoon = _platoons[_index];
			if (!_world.IsAliveEntity(_platoon)) continue;

			// ボイドが残っていなければ SwarmMissileSystem が捨てる
			_world.GetResource<SwarmMissileResource>().launchRequests.push_back(_platoon);
			++m_launchCount;
			return;
		}
	}

	void SwarmBossCoilAttackState::WriteMissileResource(SwarmBossStateContext& a_context)
	{
		auto& _world = *a_context.pObject->pWorld;
		if (!_world.HasResource<SwarmMissileResource>()) return;

		auto& _res = _world.GetResource<SwarmMissileResource>();
		_res.ringCenter    = m_center;
		_res.launchSpeed   = m_missileLaunchSpeed;
		_res.launchTime    = m_missileLaunchTime;
		_res.launchUp      = m_missileLaunchUp;
		_res.launchOut     = m_missileLaunchOut;
		_res.launchSpread  = m_missileLaunchSpread;
		_res.speed         = m_missileSpeed;
		_res.acceleration  = m_missileAcceleration;
		_res.turnSpeedDeg  = m_missileTurnSpeedDeg;
		_res.lifeTime      = m_missileLifeTime;
		_res.explodeRadius = m_missileExplodeRadius;
		_res.damage        = m_missileDamage;
		_res.color         = m_missileColor;
		_res.intensity     = m_missileIntensity;
	}

	void SwarmBossCoilAttackState::DrawRing(SwarmBossStateContext& a_context) const
	{
		if (!a_context.pObject->pServices || !a_context.pObject->pServices->pDebugDraw) return;
		auto& _debugDraw = *a_context.pObject->pServices->pDebugDraw;

		// 高さは真ん中(実際の目標点はここから上下する)
		const float _y = m_groundHeight + CalcHeight(0.0f);

		Math::Vector3 _prev = Math::Vector3(m_center.x + m_radius, _y, m_center.z);
		for (int _i = 1; _i <= RING_DRAW_SEGMENTS; ++_i)
		{
			const float _a = DirectX::XM_2PI * static_cast<float>(_i) / RING_DRAW_SEGMENTS;
			const Math::Vector3 _next(m_center.x + std::cos(_a) * m_radius, _y, m_center.z + std::sin(_a) * m_radius);
			_debugDraw.DrawLine(_prev, _next, Engine::Color::RED);
			_prev = _next;
		}
	}

	void SwarmBossCoilAttackState::Finish(SwarmBossStateContext& a_context)
	{
		if (m_phase == EPhase::End) return;
		m_phase = EPhase::End;

		if (a_context.pMachine)
		{
			a_context.pMachine->RequestChangeState(ESwarmBossState::RandomWalk);
		}
	}

	void SwarmBossCoilAttackState::Archive(Engine::Persistence::Archive& a_ar)
	{
		// ステートの調整値は同じ階層に並ぶので、名前の頭に CoilAttack を付けて区別する
		a_ar.Field("CoilAttackRadiusScale", m_radiusScale);
		a_ar.Field("CoilAttackMinRadius", m_minRadius);
		a_ar.Field("CoilAttackCenterFollowGain", m_centerFollowGain);
		a_ar.Field("CoilAttackApproachSpeedScale", m_approachSpeedScale);
		a_ar.Field("CoilAttackArriveDistance", m_arriveDistance);
		a_ar.Field("CoilAttackApproachMaxTime", m_approachMaxTime);
		a_ar.Field("CoilAttackSpeedScale", m_coilSpeedScale);
		a_ar.Field("CoilAttackFollowGain", m_followGain);
		a_ar.Field("CoilAttackTime", m_coilTime);
		a_ar.Field("CoilAttackLowHeight", m_lowHeight);
		a_ar.Field("CoilAttackHighHeight", m_highHeight);
		a_ar.Field("CoilAttackUndulationPeriod", m_undulationPeriod);
		a_ar.Field("CoilAttackLaunchStartDelay", m_launchStartDelay);
		a_ar.Field("CoilAttackLaunchInterval", m_launchInterval);

		a_ar.Field("CoilAttackMissileLaunchSpeed", m_missileLaunchSpeed);
		a_ar.Field("CoilAttackMissileLaunchTime", m_missileLaunchTime);
		a_ar.Field("CoilAttackMissileLaunchUp", m_missileLaunchUp);
		a_ar.Field("CoilAttackMissileLaunchOut", m_missileLaunchOut);
		a_ar.Field("CoilAttackMissileLaunchSpread", m_missileLaunchSpread);
		a_ar.Field("CoilAttackMissileSpeed", m_missileSpeed);
		a_ar.Field("CoilAttackMissileAcceleration", m_missileAcceleration);
		a_ar.Field("CoilAttackMissileTurnSpeedDeg", m_missileTurnSpeedDeg);
		a_ar.Field("CoilAttackMissileLifeTime", m_missileLifeTime);
		a_ar.Field("CoilAttackMissileExplodeRadius", m_missileExplodeRadius);
		a_ar.Field("CoilAttackMissileDamage", m_missileDamage);
		a_ar.Field("CoilAttackMissileColor", m_missileColor);
		a_ar.Field("CoilAttackMissileIntensity", m_missileIntensity);
	}

	void SwarmBossCoilAttackState::DrawInspector()
	{
		Engine::Editor::Field("Radius Scale", m_radiusScale, 0.01f, 0.0f);
		Engine::Editor::Tooltip("Radius = worm length / 2pi x this (1 : head meets tail)");
		Engine::Editor::Field("Min Radius", m_minRadius, 0.5f, 0.0f);
		Engine::Editor::Field("Center Follow Gain", m_centerFollowGain, 0.01f, 0.0f);
		Engine::Editor::Field("Approach Speed Scale", m_approachSpeedScale, 0.05f, 0.0f);
		Engine::Editor::Field("Arrive Distance", m_arriveDistance, 0.1f, 0.0f);
		Engine::Editor::Field("Approach Max Time", m_approachMaxTime, 0.1f, 0.0f);
		Engine::Editor::Field("Coil Speed Scale", m_coilSpeedScale, 0.05f, 0.0f);
		Engine::Editor::Tooltip("Speed scale above Platoon Scale tears the line apart");
		Engine::Editor::Field("Follow Gain", m_followGain, 0.05f, 0.0f);
		Engine::Editor::Field("Coil Time", m_coilTime, 0.1f, 0.0f);
		Engine::Editor::Field("Low Height", m_lowHeight, 0.5f);
		Engine::Editor::Tooltip("From ground surface. Negative : under ground");
		Engine::Editor::Field("High Height", m_highHeight, 0.5f);
		Engine::Editor::Field("Undulation Period", m_undulationPeriod, 0.05f, 0.0f);
		Engine::Editor::Field("Launch Start Delay", m_launchStartDelay, 0.05f, 0.0f);
		Engine::Editor::Field("Launch Interval", m_launchInterval, 0.01f, 0.01f);
		Engine::Editor::Tooltip("One boid per platoon, head to tail in order");

		Engine::Editor::Line();
		Engine::Editor::Field("Missile Launch Speed", m_missileLaunchSpeed, 0.5f, 0.0f);
		Engine::Editor::Field("Missile Launch Time", m_missileLaunchTime, 0.01f, 0.0f);
		Engine::Editor::Field("Missile Launch Up", m_missileLaunchUp, 0.01f);
		Engine::Editor::Field("Missile Launch Out", m_missileLaunchOut, 0.01f);
		Engine::Editor::Field("Missile Launch Spread", m_missileLaunchSpread, 0.01f, 0.0f);
		Engine::Editor::Field("Missile Speed", m_missileSpeed, 0.5f, 0.0f);
		Engine::Editor::Field("Missile Acceleration", m_missileAcceleration, 0.5f, 0.0f);
		Engine::Editor::Field("Missile Turn Speed", m_missileTurnSpeedDeg, 1.0f, 0.0f);
		Engine::Editor::Tooltip("deg / s. Smaller : wider turn, easier to dodge");
		Engine::Editor::Field("Missile Life Time", m_missileLifeTime, 0.1f, 0.0f);
		Engine::Editor::Field("Missile Explode Radius", m_missileExplodeRadius, 0.1f, 0.0f);
		Engine::Editor::Field("Missile Damage", m_missileDamage, 0.5f, 0.0f);
		Engine::Editor::ColorField("Missile Color", m_missileColor);
		Engine::Editor::Field("Missile Intensity", m_missileIntensity, 0.1f, 0.0f);

		// 実行中の状態は表示のみ
		Engine::Editor::Value("Phase", "%s (%.1f s)", std::string(magic_enum::enum_name(m_phase)).c_str(), m_phaseTime);
		Engine::Editor::Value("Ring", "r %.1f m / center %.1f, %.1f", m_radius, m_center.x, m_center.z);
		Engine::Editor::Value("Angle", "%.1f deg (%s)", DirectX::XMConvertToDegrees(m_angle), m_turnSign > 0.0f ? "+" : "-");
		Engine::Editor::Value("Launched", "%u (next platoon %u)", m_launchCount, static_cast<uint32_t>(m_launchCursor));
	}
}
