#include "StateMachine.h"

#include "SwarmRandomWalk/RandomWalkState.h"
#include "SwarmCharge/ChargeState.h"
#include "UperAttack/UperAttack.h"
#include "DiveAttack/DiveAttack.h"

namespace App::Object
{
	void SwarmBossStateMachine::Init()
	{
		m_upStates.clear();
		m_pCurrentState = nullptr;

		// 作ったものから足していく
		m_upStates.emplace(ESwarmBossState::RandomWalk, std::make_unique<SwarmBossRandomWalkState>());
		m_upStates.emplace(ESwarmBossState::Charge, std::make_unique<SwarmBossChargeState>());
		m_upStates.emplace(ESwarmBossState::UperAttack, std::make_unique<SwarmBossUperAttackState>());
		m_upStates.emplace(ESwarmBossState::DiveAttack, std::make_unique<SwarmBossDiveAttackState>());

		// 最初の行動。1フレーム目の PreUpdate で入る
		RequestChangeState(ESwarmBossState::RandomWalk);
	}

	void SwarmBossStateMachine::PreUpdate(SwarmBossStateContext& a_context)
	{
		if (m_isChangeRequested)
		{
			ChangeState(a_context);
		}
	}

	void SwarmBossStateMachine::Update(SwarmBossStateContext& a_context)
	{
		if (m_pCurrentState)
		{
			m_pCurrentState->Update(a_context);
		}
	}

	void SwarmBossStateMachine::PostUpdate(SwarmBossStateContext& a_context)
	{}

	void SwarmBossStateMachine::RequestChangeState(ESwarmBossState a_state)
	{
		m_changeState = a_state;
		m_isChangeRequested = true;
	}

	void SwarmBossStateMachine::ChangeState(SwarmBossStateContext& a_context)
	{
		m_isChangeRequested = false;

		auto _it = m_upStates.find(m_changeState);
		if (_it == m_upStates.end() || !_it->second)
		{
			// まだ作っていないステート。今のステートを続ける
			ENGINE_WARNING("SwarmBossStateMachine : 未登録のステートです : %s",
				std::string(magic_enum::enum_name(m_changeState)).c_str());
			return;
		}

		if (m_pCurrentState)
		{
			m_pCurrentState->Exit(a_context);
		}

		m_currentState  = m_changeState;
		m_pCurrentState = _it->second.get();
		m_pCurrentState->Enter(a_context);
	}

	void SwarmBossStateMachine::Archive(Engine::Persistence::Archive& a_ar)
	{
		for (auto& [_state, _upState] : m_upStates)
		{
			if (_upState) _upState->Archive(a_ar);
		}
	}

	void SwarmBossStateMachine::DrawInspector()
	{
		Engine::Editor::Value("State", "%s", m_pCurrentState
			? std::string(magic_enum::enum_name(m_currentState)).c_str()
			: "(not started)");

		for (auto& [_state, _upState] : m_upStates)
		{
			if (!_upState) continue;

			Engine::Editor::Header(std::string(magic_enum::enum_name(_state)).c_str());
			Engine::Editor::IDScope _id(static_cast<int>(_state));
			_upState->DrawInspector();
		}
	}
}
