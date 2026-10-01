#include "Reorganize.h"

// エンジン
#include "Engine/GameObject/BaseObject/BaseObject.h"
#include "Engine/Persistence/Archive/Archive.h"
#include "Engine/ECS/System/SystemContext.h"
#include "Engine/Editor/Helper/EditorField.h"	// コンポーネントの Traits が使うので先に置く

// App
#include "../../../../../ECS/World/APPWorld.h"
#include "../StateMachine.h"

#include "Application/Components/Movement/MoveIntentComponent.h"

namespace App::Object
{
	void SwarmBossReorganizeState::Enter(SwarmBossStateContext& a_context)
	{
		ChangePhase(EPhase::Gather);
		m_isRequested = false;

		// 高さは今のまま(地面に埋まる分だけ持ち上げる)
		m_ball.Begin(a_context);
	}

	void SwarmBossReorganizeState::Update(SwarmBossStateContext& a_context)
	{
		if (m_phase == EPhase::End) return;
		if (!a_context.pObject || !a_context.pObject->pWorld) return;
		const float _dt = a_context.pObject->dt;

		// 整理している間は体を無敵にする(抜ければ依頼の既定値 1 に戻る)
		a_context.bodyDefenseRatio = m_defenseRatio;

		// どのフェーズでも球の表面を回り続ける
		m_ball.Update(a_context, _dt);
		m_ball.Draw(a_context);

		m_phaseTime += _dt;

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
		m_ball.Archive(a_ar, "Reorganize");
		a_ar.Field("ReorganizeGatherTime", m_gatherTime);
		a_ar.Field("ReorganizeSettleTime", m_settleTime);
		a_ar.Field("ReorganizeDefenseRatio", m_defenseRatio);
	}

	void SwarmBossReorganizeState::DrawInspector()
	{
		m_ball.DrawInspector();
		Engine::Editor::Field("Gather Time", m_gatherTime, 0.05f, 0.0f);
		Engine::Editor::Field("Settle Time", m_settleTime, 0.05f, 0.0f);
		Engine::Editor::Field("Defense Ratio", m_defenseRatio, 0.01f, 0.0f, 1.0f);
		Engine::Editor::Tooltip("Body damage ratio while reorganizing (0 : invincible)");

		// 実行中の状態は表示のみ
		Engine::Editor::Value("Phase", "%s (%.1f s)", std::string(magic_enum::enum_name(m_phase)).c_str(), m_phaseTime);
		Engine::Editor::Value("Requested", "%s", m_isRequested ? "yes" : "no");
	}
}
