#include "ScoreHUD.h"

#include "Engine/ECS/Internal/SystemContext.h"	// ObjectContext が運ぶサービス群
#include "Engine/MainEngine.h"
#include "Engine/Graphics/GraphicEngine.h"
#include "Engine/ECS/World/World.h"
#include "Engine/Editor/Helper/EditorHelper.h"

#include "Application/Game/GameManager/GameManager.h"

namespace App::Object
{
	//======================================================================================
	// 出す数を GlobalGameContext から取る
	//======================================================================================
	int ScoreHUD::PickValue() const
	{
		const auto& _gameData = App::Game::GameManager::Instance().GetGameData();

		switch (m_valueKind)
		{
		case EScoreValueKind::Time:
			// 秒だけ出す。小数は切り捨て(桁が細かく動くと読めないため)
			return static_cast<int>(std::max(_gameData.time, 0.0f));

		case EScoreValueKind::KillCount:
			return _gameData.killCount;

		case EScoreValueKind::Score:
		default:
			return _gameData.score;
		}
	}

	void ScoreHUD::Update(Engine::GameObject::ObjectContext& a_context)
	{
		// 戻りを進める
		if (m_punchTimer > 0.0f)
		{
			m_punchTimer = std::max(m_punchTimer - a_context.dt, 0.0f);
		}

		m_value = PickValue();

		//==================================================================
		// 増えたフレームだけ弾ませる
		//------------------------------------------------------------------
		// 前フレームの数と比べるだけにしてある。加算した側に
		// 「このフレームで入ったぶん」を持たせると、
		// 加算する側が居ないシーン(リザルト)で立ちっぱなしになるため。
		//
		// 出た最初のフレームは比べる相手が無いので弾ませない
		// (リザルトへ来た瞬間にスコアが跳ねてしまう)
		//==================================================================
		if (!m_isFirst && m_value > m_prevValue)
		{
			m_punchTimer = m_punchTime;
		}

		m_prevValue = m_value;
		m_isFirst = false;
	}

	void ScoreHUD::DrawDigit(
		Engine::GameObject::ObjectContext& a_context,
		int a_digit,
		int a_index,
		float a_scale)
	{
		auto* _pGE = a_context.pServices->pMainEngine->RefGraphicsEngine();
		if (!_pGE) return;

		const Math::Vector2 _size = m_pixelSize * a_scale;

		// 桁の送りは「元の大きさ」で決める。
		// 弾んでいる間の大きさで送ると、拡大するたびに桁が横へ広がってしまう
		const float _step = m_pixelSize.x + m_digitSpacing;

		Math::Vector2 _pos = m_pixelPos;
		_pos.x += _step * static_cast<float>(a_index);

		//--------------------------------------------------------------
		// 1枚に並んだ数字から1コマだけ切り出す
		//
		// uv * uvScale + uvOffset なので、倍率がコマの幅、
		// オフセットが「何コマ目か」になる
		//--------------------------------------------------------------
		const float _cellWidth = 1.0f / static_cast<float>(m_atlasCount);

		const Math::Vector2 _uvScale  = { _cellWidth, 1.0f };
		const Math::Vector2 _uvOffset = { _cellWidth * static_cast<float>(a_digit), 0.0f };

		_pGE->SubmitUI(
			m_texRef,
			_pos,
			_size,
			m_color,
			m_rotation,
			m_layer,
			_uvOffset,
			m_pivot,
			_uvScale
		);
	}

	void ScoreHUD::Draw(Engine::GameObject::ObjectContext& a_context)
	{
		if (!a_context.pServices || !a_context.pServices->pMainEngine) return;
		if (m_digitCount <= 0 || m_atlasCount <= 0) return;

		// 弾みの割合(1 → 0)。増えた瞬間が 1
		float _rate = 0.0f;
		if (m_punchTime > 1e-4f)
		{
			_rate = std::clamp(m_punchTimer / m_punchTime, 0.0f, 1.0f);
		}
		const float _scale = 1.0f + (m_punchScale - 1.0f) * _rate;

		//--------------------------------------------------------------
		// 桁を左から並べる
		//
		// 上の桁から順に取り出す。表示桁を超えたぶんは出せないので、
		// 一番上の桁で頭打ちにする(999999 のまま止まる)
		//--------------------------------------------------------------
		int _value = std::max(m_value, 0);

		// 表示できる最大値でクランプ(10^digitCount - 1)
		int _max = 1;
		for (int _i = 0; _i < m_digitCount; ++_i)
		{
			_max *= 10;

			// 桁数を大きくしすぎたときに int があふれないよう止める
			if (_max >= 1000000000) break;
		}
		_value = std::min(_value, _max - 1);

		int _divisor = _max / 10;
		for (int _i = 0; _i < m_digitCount; ++_i)
		{
			if (_divisor <= 0) break;

			const int _digit = (_value / _divisor) % 10;
			DrawDigit(a_context, _digit, _i, _scale);

			_divisor /= 10;
		}
	}

	void ScoreHUD::Archive(Engine::Persistence::Archive& a_ar, Engine::GameObject::ObjectContext& a_context)
	{
		// テクスチャ・色・サイズなどの共通ぶん
		UIBase::Archive(a_ar, a_context);

		a_ar.Field("ValueKind", m_valueKind);
		a_ar.Field("DigitCount", m_digitCount);
		a_ar.Field("AtlasCount", m_atlasCount);
		a_ar.Field("DigitSpacing", m_digitSpacing);
		a_ar.Field("PunchScale", m_punchScale);
		a_ar.Field("PunchTime", m_punchTime);
	}

	void ScoreHUD::DrawInspector(Engine::GameObject::ObjectContext& a_context)
	{
		UIBase::DrawInspector(a_context);

		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();

		ImGui::Text("Score");
		ImGui::TextDisabled("0〜9 を横一列に並べた1枚のテクスチャを指定すること");

		// 何を出すか。中身は GlobalGameContext から貰う
		Engine::Editor::EditorHelper::DrawEnumCombo("ValueKind", m_valueKind);
		ImGui::TextDisabled("Time は秒だけ(小数は切り捨て)");

		if (ImGui::DragInt("DigitCount", &m_digitCount, 1, 1, 9))
		{
			m_digitCount = std::clamp(m_digitCount, 1, 9);
		}
		ImGui::TextDisabled("表示する桁数。足りないぶんは 0 で埋める");

		if (ImGui::DragInt("AtlasCount", &m_atlasCount, 1, 1, 64))
		{
			m_atlasCount = std::max(m_atlasCount, 1);
		}
		ImGui::TextDisabled("テクスチャに並んでいるコマ数(0〜9 だけなら 10)");

		ImGui::DragFloat("DigitSpacing", &m_digitSpacing, 0.5f);
		ImGui::TextDisabled("桁と桁の間隔(px)。PixelSize が1桁ぶんの大きさ");

		ImGui::Separator();
		ImGui::DragFloat("PunchScale", &m_punchScale, 0.01f, 1.0f, 4.0f);
		ImGui::DragFloat("PunchTime", &m_punchTime, 0.01f, 0.0f, 2.0f);
		ImGui::TextDisabled("スコアが増えたフレームだけ大きくして戻す");

		ImGui::Separator();

		// 中身はシーンをまたぐグローバル側。ここでは表示と確認だけ
		auto& _gameData = App::Game::GameManager::Instance().RefGameData();

		ImGui::Text("Value : %d", m_value);
		ImGui::Text("Score : %d", _gameData.score);
		ImGui::Text("Kill  : %d", _gameData.killCount);
		ImGui::Text("Time  : %.2f", _gameData.time);

		// 実際に倒さなくても並びを確かめられるようにしておく
		if (Engine::Editor::EditorHelper::CreateButton("Add 100"))
		{
			_gameData.AddScore(100);
		}
		ImGui::SameLine();
		if (Engine::Editor::EditorHelper::DeleteButton("Reset"))
		{
			_gameData.ResetRun();
		}
	}
}
