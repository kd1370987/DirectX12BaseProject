#include "Death.h"

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
	void SwarmBossDeathState::Enter(SwarmBossStateContext& a_context)
	{
		m_phase          = EPhase::Gather;
		m_phaseTime      = 0.0f;
		m_waveSpeedScale = 1.0f;

		// 中心は地表から決まった高さ
		m_ball.Begin(a_context, std::max(m_centerHeight, 0.0f));
	}

	void SwarmBossDeathState::Update(SwarmBossStateContext& a_context)
	{
		if (m_phase == EPhase::Burst) return;
		if (!a_context.pObject || !a_context.pObject->pWorld) return;
		const float _dt = a_context.pObject->dt;

		m_phaseTime += _dt;

		//--------------------------------------------------------------
		// まとまる : 経過の割合の2乗で、だんだん速く締めていく
		//--------------------------------------------------------------
		const float _t    = (m_gatherTime > 0.0f) ? std::clamp(m_phaseTime / m_gatherTime, 0.0f, 1.0f) : 1.0f;
		const float _ease = _t * _t;

		m_waveSpeedScale = std::lerp(1.0f, std::max(m_waveSpeedScaleMax, 1.0f), _ease);

		a_context.bodyDefenseRatio = m_defenseRatio;
		a_context.waveSpeedScale   = m_waveSpeedScale;

		m_ball.Update(a_context, _dt, std::lerp(1.0f, std::max(m_endRadiusScale, 0.0f), _ease));
		m_ball.Draw(a_context);

		if (_t < 1.0f) return;

		//--------------------------------------------------------------
		// 爆散 : 球の中心から外へ飛ばすよう頼む(頼むのは一度だけ)
		//--------------------------------------------------------------
		a_context.burst = m_burst;
		a_context.burst.isRequested = true;
		a_context.burst.center      = m_ball.GetCenter();

		m_phase     = EPhase::Burst;
		m_phaseTime = 0.0f;
	}

	void SwarmBossDeathState::Exit(SwarmBossStateContext& a_context)
	{
		// 抜けることは無い想定だが、入力は残さない
		if (!a_context.pObject || !a_context.pObject->pWorld) return;
		auto& _world = *a_context.pObject->pWorld;

		const auto _leader = a_context.leaderEntity;
		if (!_world.IsAliveEntity(_leader)) return;
		if (!_world.HasComponent<MoveIntentComponent>(_leader)) return;

		_world.RefData<MoveIntentComponent>(_leader)->value = Math::Vector3(0.0f, 0.0f, 0.0f);
	}

	void SwarmBossDeathState::Archive(Engine::Persistence::Archive& a_ar)
	{
		// ステートの調整値は同じ階層に並ぶので、名前の頭に Death を付けて区別する
		m_ball.Archive(a_ar, "Death");
		a_ar.Field("DeathCenterHeight", m_centerHeight);
		a_ar.Field("DeathEndRadiusScale", m_endRadiusScale);
		a_ar.Field("DeathGatherTime", m_gatherTime);
		a_ar.Field("DeathWaveSpeedScaleMax", m_waveSpeedScaleMax);
		a_ar.Field("DeathDefenseRatio", m_defenseRatio);

		a_ar.Field("DeathBurstSpeedMin", m_burst.speedMin);
		a_ar.Field("DeathBurstSpeedMax", m_burst.speedMax);
		a_ar.Field("DeathBurstUpBias", m_burst.upBias);
		a_ar.Field("DeathBurstLifeMin", m_burst.lifeMin);
		a_ar.Field("DeathBurstLifeMax", m_burst.lifeMax);
		a_ar.Field("DeathBurstGravity", m_burst.gravity);
		a_ar.Field("DeathBurstDrag", m_burst.drag);
		a_ar.Field("DeathBurstColor", m_burst.color);
		a_ar.Field("DeathBurstIntensity", m_burst.intensity);
	}

	void SwarmBossDeathState::DrawInspector()
	{
		m_ball.DrawInspector();
		Engine::Editor::Field("Center Height", m_centerHeight, 0.5f, 0.0f);
		Engine::Editor::Tooltip("Height of the ball center above the ground");
		Engine::Editor::Field("End Radius Scale", m_endRadiusScale, 0.01f, 0.0f);
		Engine::Editor::Field("Gather Time", m_gatherTime, 0.1f, 0.0f);
		Engine::Editor::Field("Wave Speed Scale Max", m_waveSpeedScaleMax, 0.1f, 1.0f);
		Engine::Editor::Tooltip("Wave speed x this (interval / this) just before the burst. Ramps up by t^2");
		Engine::Editor::Field("Defense Ratio", m_defenseRatio, 0.01f, 0.0f, 1.0f);
		Engine::Editor::Tooltip("Body damage ratio while gathering (0 : invincible)");

		Engine::Editor::Line();
		Engine::Editor::Field("Burst Speed Min", m_burst.speedMin, 0.5f, 0.0f);
		Engine::Editor::Field("Burst Speed Max", m_burst.speedMax, 0.5f, 0.0f);
		Engine::Editor::Field("Burst Up Bias", m_burst.upBias, 0.01f, 0.0f);
		Engine::Editor::Field("Burst Life Min", m_burst.lifeMin, 0.01f, 0.0f);
		Engine::Editor::Field("Burst Life Max", m_burst.lifeMax, 0.01f, 0.0f);
		Engine::Editor::Field("Burst Gravity", m_burst.gravity, 0.5f, 0.0f);
		Engine::Editor::Field("Burst Drag", m_burst.drag, 0.01f, 0.0f);
		Engine::Editor::ColorField("Burst Color", m_burst.color);
		Engine::Editor::Field("Burst Intensity", m_burst.intensity, 0.1f, 0.0f);

		// 実行中の状態は表示のみ
		Engine::Editor::Value("Phase", "%s (%.1f s)", std::string(magic_enum::enum_name(m_phase)).c_str(), m_phaseTime);
		Engine::Editor::Value("Wave Scale", "x %.2f", m_waveSpeedScale);
	}
}
