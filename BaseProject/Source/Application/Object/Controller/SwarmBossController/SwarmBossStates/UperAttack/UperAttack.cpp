#include "UperAttack.h"

// エンジン
#include "Engine/GameObject/BaseObject/BaseObject.h"
#include "Engine/Persistence/Archive/Archive.h"
#include "Engine/ECS/System/SystemContext.h"
#include "Engine/Editor/Helper/EditorHelper.h"	// コンポーネントの Traits が使うので先に置く

// App
#include "../../../../../ECS/World/APPWorld.h"
#include "../StateMachine.h"

#include "../../../../../Components/Transform/LocalTransformComponent.h"
#include "../../../../../Components/Transform/WorldMatrixComponent.h"
#include "../../../../../Components/Intent/MoveIntentComponent.h"
#include "../../../../../Components/Tag/PlayerControllTag.h"

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
	}

	void SwarmBossUperAttackState::Enter(SwarmBossStateContext& a_context)
	{
		m_phase = EPhase::Windup;
		m_phaseTime = 0.0f;
	}

	void SwarmBossUperAttackState::Update(SwarmBossStateContext& a_context)
	{
		if (m_phase == EPhase::End) return;
		if (!a_context.pObject || !a_context.pObject->pWorld) return;
		auto& _world = *a_context.pObject->pWorld;
		const float _dt = a_context.pObject->dt;

		const auto _leader = a_context.leaderEntity;
		if (!_world.IsAliveEntity(_leader)) return;
		if (!_world.HasComponent<MoveIntentComponent>(_leader)) return;
		if (!_world.HasComponent<LocalTransformComponent>(_leader)) return;

		const Math::Vector3 _pos = _world.RefData<LocalTransformComponent>(_leader)->pos;

		// プレイヤーが居ない(倒された / まだ湧いていない)なら突っ込む先が無い
		const bool _isFoundPlayer = FindPlayerPos(*a_context.pObject, m_playerPos);
		if (!_isFoundPlayer && m_phase != EPhase::Recover)
		{
			Finish(a_context);
			return;
		}

		// プレイヤーへの向き
		Math::Vector3 _toPlayerDir = m_playerPos - _pos;
		const float _distToPlayer = _toPlayerDir.Length();
		_toPlayerDir.Normalize();

		// 慣性
		Math::Vector3 _intent = Math::Vector3(0.0f, 0.0f, 0.0f);

		switch (m_phase)
		{
		//--------------------------------------------------------------
		// 溜め : プレイヤーの真下の地面まで行く
		//--------------------------------------------------------------
		case App::Object::SwarmBossUperAttackState::EPhase::Windup:
			_intent = _toPlayerDir * std::clamp(m_windupThrottle, 0.0f, 1.0f);

			// 制限時間までに到達できなかったらその場で
			if (m_phaseTime >= m_windupTime)
			{
				m_phase = EPhase::Uper;
				m_phaseTime = 0.0f;
			}
			break;
		//--------------------------------------------------------------
		// 突進 : 真上に向かって飛び出す
		//--------------------------------------------------------------
		case App::Object::SwarmBossUperAttackState::EPhase::Uper:
			_intent = Math::Vector3::Up() * std::max(m_chargeSpeedScale, 0.0f);

			// 通り過ぎて十分離れたか、時間切れで終わり
			if (m_phaseTime >= m_maxChargeTime)
			{
				m_phase = EPhase::Recover;
				m_phaseTime = 0.0f;
			}

			break;
		//--------------------------------------------------------------
		// 余韻 : 同じ向きへ惰性で進んでから徘徊へ戻る
		//--------------------------------------------------------------
		case App::Object::SwarmBossUperAttackState::EPhase::Recover:
			_intent = Math::Vector3::Up() * std::clamp(m_recoverThrottle, 0.0f, 1.0f);

			if (m_phaseTime >= m_recoverTime)
			{
				Finish(a_context);
				return;
			}
			break;
		default:
			break;
		}
	}

	void SwarmBossUperAttackState::Exit(SwarmBossStateContext& a_context)
	{
		// 突進の速さの入力を次のステートへ持ち越さない
		if (!a_context.pObject || !a_context.pObject->pWorld) return;
		auto& _world = *a_context.pObject->pWorld;

		const auto _leader = a_context.leaderEntity;
		if (!_world.IsAliveEntity(_leader)) return;
		if (!_world.HasComponent<MoveIntentComponent>(_leader)) return;

		_world.RefData<MoveIntentComponent>(_leader)->value = Math::Vector3(0.0f, 0.0f, 0.0f);
	}

	void SwarmBossUperAttackState::Archive(Engine::Persistence::Archive& a_ar)
	{// ステートの調整値は同じ階層に並ぶので、名前の頭に Charge を付けて区別する
		a_ar.Field("UperAttackWindupTime", m_windupTime);
		a_ar.Field("UperAttackWindupThrottle", m_windupThrottle);
		a_ar.Field("UperAttackSpeedScale", m_chargeSpeedScale);
		a_ar.Field("UperAttackHomingTurnSpeed", m_homingTurnSpeed);
		a_ar.Field("UperAttackMaxTime", m_maxChargeTime);
		a_ar.Field("UperAttackOvershootDistance", m_overshootDistance);
		a_ar.Field("UperAttackRecoverTime", m_recoverTime);
		a_ar.Field("UperAttackRecoverThrottle", m_recoverThrottle);
	}

	void SwarmBossUperAttackState::DrawInspector()
	{
		ImGui::DragFloat("Windup Time", &m_windupTime, 0.05f, 0.0f);
		ImGui::DragFloat("Windup Throttle", &m_windupThrottle, 0.01f, 0.0f, 1.0f);
		ImGui::DragFloat("Charge Speed Scale", &m_chargeSpeedScale, 0.05f, 0.0f);
		ImGui::DragFloat("Homing Turn Speed", &m_homingTurnSpeed, 0.01f, 0.0f);
		ImGui::DragFloat("Max Charge Time", &m_maxChargeTime, 0.1f, 0.0f);
		ImGui::DragFloat("Overshoot Distance", &m_overshootDistance, 0.5f, 0.0f);
		ImGui::DragFloat("Recover Time", &m_recoverTime, 0.05f, 0.0f);
		ImGui::DragFloat("Recover Throttle", &m_recoverThrottle, 0.01f, 0.0f, 1.0f);
		ImGui::TextDisabled("Speed scale above Platoon Scale tears the line apart");

		// 実行中の状態は表示のみ
		ImGui::Text("Phase   : %s (%.1f s)", std::string(magic_enum::enum_name(m_phase)).c_str(), m_phaseTime);
		ImGui::Text("Player  : %.1f, %.1f, %.1f", m_playerPos.x, m_playerPos.y, m_playerPos.z);
	}

	void SwarmBossUperAttackState::Finish(SwarmBossStateContext& a_context)
	{
		if (m_phase == EPhase::End) return;
		m_phase = EPhase::End;

		if (a_context.pMachine)
		{
			a_context.pMachine->RequestChangeState(ESwarmBossState::RandomWalk);
		}
	}

}