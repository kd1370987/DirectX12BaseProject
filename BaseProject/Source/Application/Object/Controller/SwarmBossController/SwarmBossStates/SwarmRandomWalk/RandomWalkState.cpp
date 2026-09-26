#include "RandomWalkState.h"

// エンジン
#include "Engine/GameObject/BaseObject/BaseObject.h"
#include "Engine/Persistence/Archive/Archive.h"
#include "Engine/ECS/System/SystemContext.h"
#include "Engine/Editor/Helper/EditorField.h"	// コンポーネントの Traits が使うので先に置く

// App
#include "../../../../../ECS/World/APPWorld.h"
#include "../StateMachine.h"

#include "../../../../../Components/Transform/LocalTransformComponent.h"
#include "../../../../../Components/Intent/MoveIntentComponent.h"

namespace App::Object
{
	void SwarmBossRandomWalkState::Enter(SwarmBossStateContext& a_context)
	{
		// 入った瞬間に行き先を決める(前のステートの目標を引きずらない)
		PickWanderTarget(a_context.spawnPos);

		// 計測開始。攻撃へ移る時間はここで決める
		m_time = 0.0f;
		m_attackTime = Math::Random::Float(
			std::min(m_minDurationTime, m_maxDurationTime),
			std::max(m_minDurationTime, m_maxDurationTime));

		// 何を出すかもここで決めておく
		m_nextAttack = PickAttack();
	}

	void SwarmBossRandomWalkState::Update(SwarmBossStateContext& a_context)
	{
		if (!a_context.pObject || !a_context.pObject->pWorld) return;
		auto& _world = *a_context.pObject->pWorld;

		m_time += a_context.pObject->dt;

		// 時間が来たら攻撃へ(切り替わるのは次のフレーム。それまでは徘徊を続ける)
		if (m_time >= m_attackTime && a_context.pMachine)
		{
			a_context.pMachine->RequestChangeState(m_nextAttack);
		}

		const auto _leader = a_context.leaderEntity;
		if (!_world.IsAliveEntity(_leader)) return;
		if (!_world.HasComponent<MoveIntentComponent>(_leader)) return;
		if (!_world.HasComponent<LocalTransformComponent>(_leader)) return;

		const Math::Vector3 _pos = _world.RefData<LocalTransformComponent>(_leader)->pos;

		// 目標地点を選び直すか
		m_wanderTimer -= a_context.pObject->dt;

		const Math::Vector3 _toTarget = m_targetPos - _pos;
		if (m_wanderTimer <= 0.0f || _toTarget.Length() <= m_arriveDistance)
		{
			PickWanderTarget(a_context.spawnPos);
		}

		// 目標地点へ向かう入力。長さがスロットルになる(向きは世界空間)
		Math::Vector3 _dir = m_targetPos - _pos;
		if (_dir.LengthSquared() > 1e-6f)
		{
			_dir.Normalize();
			_dir *= std::clamp(m_throttle, 0.0f, 1.0f);
		}
		else
		{
			_dir = Math::Vector3(0.0f, 0.0f, 0.0f);
		}

		_world.RefData<MoveIntentComponent>(_leader)->value = _dir;
	}

	void SwarmBossRandomWalkState::Exit(SwarmBossStateContext& a_context)
	{
		// 入力を残したまま抜けると、次のステートが書かない限り走り続けるので止めておく
		if (!a_context.pObject || !a_context.pObject->pWorld) return;
		auto& _world = *a_context.pObject->pWorld;

		const auto _leader = a_context.leaderEntity;
		if (!_world.IsAliveEntity(_leader)) return;
		if (!_world.HasComponent<MoveIntentComponent>(_leader)) return;

		_world.RefData<MoveIntentComponent>(_leader)->value = Math::Vector3(0.0f, 0.0f, 0.0f);
	}

	void SwarmBossRandomWalkState::PickWanderTarget(const Math::Vector3& a_center)
	{
		// 水平は円の中から、高さは振れ幅の中から選ぶ。
		// 円内の一様分布にするため半径は平方根を取る(そのまま掛けると中心に寄る)
		const float _angle  = Math::Random::Float(0.0f, DirectX::XM_2PI);
		const float _radius = m_wanderRadius * std::sqrt(Math::Random::Float(0.0f, 1.0f));

		m_targetPos = a_center + Math::Vector3(
			std::cos(_angle) * _radius,
			Math::Random::Float(-m_wanderHeight, m_wanderHeight),
			std::sin(_angle) * _radius);

		m_wanderTimer = m_wanderInterval;
	}

	ESwarmBossState SwarmBossRandomWalkState::PickAttack() const
	{
		// 攻撃と重みの組。足すときはここに並べる
		const std::pair<ESwarmBossState, float> _table[] =
		{
			{ ESwarmBossState::Charge,     std::max(m_chargeWeight, 0.0f) },
			{ ESwarmBossState::UperAttack, std::max(m_uperAttackWeight, 0.0f) },
			{ ESwarmBossState::DiveAttack, std::max(m_diveAttackWeight, 0.0f) },
		};

		float _total = 0.0f;
		for (const auto& [_state, _weight] : _table) _total += _weight;
		if (_total <= 0.0f) return ESwarmBossState::Charge;

		// 0〜合計の中で引いた値が、どの重みの区間に入ったか
		float _pick = Math::Random::Float(0.0f, _total);
		for (const auto& [_state, _weight] : _table)
		{
			if (_pick < _weight) return _state;
			_pick -= _weight;
		}

		// 浮動小数の誤差で最後を越えたときは、重みを持つ最後のもの
		for (auto _it = std::rbegin(_table); _it != std::rend(_table); ++_it)
		{
			if (_it->second > 0.0f) return _it->first;
		}
		return ESwarmBossState::Charge;
	}

	void SwarmBossRandomWalkState::Archive(Engine::Persistence::Archive& a_ar)
	{
		// コントローラーに直に持っていた頃と同じ名前(既存シーンをそのまま読める)
		// 目標地点は走り出してから抽選するので保存しない
		a_ar.Field("WanderRadius", m_wanderRadius);
		a_ar.Field("WanderHeight", m_wanderHeight);
		a_ar.Field("WanderInterval", m_wanderInterval);
		a_ar.Field("ArriveDistance", m_arriveDistance);
		a_ar.Field("Throttle", m_throttle);
		a_ar.Field("AttackIntervalMin", m_minDurationTime);
		a_ar.Field("AttackIntervalMax", m_maxDurationTime);
		a_ar.Field("AttackWeightCharge", m_chargeWeight);
		a_ar.Field("AttackWeightUperAttack", m_uperAttackWeight);
		a_ar.Field("AttackWeightDiveAttack", m_diveAttackWeight);
	}

	void SwarmBossRandomWalkState::DrawInspector()
	{
		Engine::Editor::Field("Wander Radius", m_wanderRadius, 0.5f, 0.0f);
		Engine::Editor::Field("Wander Height", m_wanderHeight, 0.5f, 0.0f);
		Engine::Editor::Field("Wander Interval", m_wanderInterval, 0.1f, 0.0f);
		Engine::Editor::Field("Arrive Distance", m_arriveDistance, 0.1f, 0.0f);
		Engine::Editor::Field("Throttle", m_throttle, 0.01f, 0.0f, 1.0f);
		Engine::Editor::Field("Attack Interval Min", m_minDurationTime, 0.1f, 0.0f);
		Engine::Editor::Field("Attack Interval Max", m_maxDurationTime, 0.1f, 0.0f);
		Engine::Editor::Field("Weight Charge", m_chargeWeight, 0.05f, 0.0f);
		Engine::Editor::Field("Weight Uper Attack", m_uperAttackWeight, 0.05f, 0.0f);
		Engine::Editor::Field("Weight Dive Attack", m_diveAttackWeight, 0.05f, 0.0f);

		// 目標地点は毎フレーム上書きされるので表示のみ
		Engine::Editor::Value("Target", "%.1f, %.1f, %.1f (next %.1f s)", m_targetPos.x, m_targetPos.y, m_targetPos.z, m_wanderTimer);
		Engine::Editor::Value("Attack", "%.1f / %.1f s -> %s", m_time, m_attackTime, std::string(magic_enum::enum_name(m_nextAttack)).c_str());
	}
}
