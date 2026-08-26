#include "UIGauge.h"

#include "Engine/ECS/Internal/SystemContext.h"	// ObjectContext が運ぶサービス群
#include "Application/ECS/World/World.h"
#include "Engine/Editor/Helper/EditorHelper.h"

#include "Application/Components/Tag/PlayerControllTag.h"
#include "Application/Components/Character/HealthComponent.h"
#include "Application/Components/Character/LockOnTargetComponent.h"
#include "Application/Components/Character/Robot/BoostComponent.h"
#include "Application/Components/Character/Robot/ChargeDashComponent.h"
#include "Application/Components/Character/Robot/AttachmentSlotsComponent.h"
#include "Application/Components/Character/Weapon/Gun/GunStateComponent.h"

//==========================================================================================
// UIGauge
//
// 現在値と最大値だけを受け取り、残量で横幅と色を変える。
//
// ・中身は飾り1つ
//     名前で指す。飾りなので、絵でも板ポリでも、枠付きでも同じように縮む。
//     縮めるのは DrawOverride の sizeScale(大きさだけに掛かる倍率)なので、
//     飾り側の値は書き換えない = 次のフレームへ汚れが残らない。
//
// ・ピボットだけは飾りへ書き込む
//     「左端を固定して右から減る」はピボットが左端(0)であることが前提。
//     ここを合わせておかないと中央から両側へ縮んでしまうので、
//     設定した向きに合わせて毎フレーム同じ値を入れ直している。
//     毎フレーム動く値ではないため、汚れとしては残らない。
//==========================================================================================
namespace App::Object
{
	//======================================================================================
	// 値
	//======================================================================================
	void UIGauge::SetValue(float a_current, float a_max)
	{
		m_max = a_max;
		m_current = a_current;
	}

	void UIGauge::SetCurrent(float a_current)
	{
		m_current = a_current;
	}

	float UIGauge::GetRatio() const
	{
		// 最大値が入っていないものは空として扱う。
		// 0除算を避けるためだけでなく、「まだ値が入っていない」ことを
		// 満タンで見せてしまわないため
		if (m_max <= 0.0f) return 0.0f;

		return std::clamp(m_current / m_max, 0.0f, 1.0f);
	}

	//======================================================================================
	// 更新
	//======================================================================================
	void UIGauge::Update(Engine::GameObject::ObjectContext& a_context)
	{
		// カーソルの判定と飾りのアニメーション
		UIBase::Update(a_context);

		//------------------------------------------------------------------
		// 値の取り込み
		//
		// 見る相手も値も毎フレーム引き直す。覚え込むと、
		// ロックが外れた・敵が消えた・武器を持ち替えた のいずれも取りこぼす
		//------------------------------------------------------------------
		if (m_source == EGaugeSource::Manual)
		{
			// 入れるのは外(SetValue)。ここは何もしない
			m_hasValue = true;
		}
		else
		{
			UpdateTargetEntity(a_context.pWorld);
			m_hasValue = FetchValue(a_context.pWorld);
		}

		// 伸び縮みの向きへピボットを合わせる
		ApplyFillPivot();

		// 数値の流し込み
		ApplyValueText();
	}

	//======================================================================================
	// 見るエンティティを決める
	//======================================================================================
	void UIGauge::UpdateTargetEntity(Engine::ECS::World* a_pWorld)
	{
		if (a_pWorld == nullptr)
		{
			m_targetEntity = Engine::ECS::Limits::INVALID_ENTITY;
			return;
		}

		switch (m_target)
		{
		case EGaugeTarget::Player:
			m_targetEntity = FindPlayer(a_pWorld);
			break;

		case EGaugeTarget::LockedEnemy:
		{
			m_targetEntity = Engine::ECS::Limits::INVALID_ENTITY;

			// ロック結果はプレイヤーが持っている。
			// 射影も判定も LockOnTargetSystem が済ませてあるので、ここは読むだけ
			const Engine::ECS::Entity _player = FindPlayer(a_pWorld);
			if (_player == Engine::ECS::Limits::INVALID_ENTITY) break;
			if (!a_pWorld->HasComponent<LockOnTargetComponent>(_player)) break;

			const auto* _pLockOn = a_pWorld->RefData<LockOnTargetComponent>(_player);
			if (_pLockOn && _pLockOn->IsLocked()) m_targetEntity = _pLockOn->lockedEntity;
			break;
		}

		case EGaugeTarget::PlayerRightWeapon:
			m_targetEntity = FindPlayerWeapon(a_pWorld, true);
			break;

		case EGaugeTarget::PlayerLeftWeapon:
			m_targetEntity = FindPlayerWeapon(a_pWorld, false);
			break;

		case EGaugeTarget::Manual:
		default:
			// 外から入れられたものをそのまま使う。
			// 相手が消えていたら掴んだままにしない(別のものが同じIDで再利用される)
			if (!a_pWorld->IsAliveEntity(m_targetEntity))
			{
				m_targetEntity = Engine::ECS::Limits::INVALID_ENTITY;
			}
			break;
		}
	}

	//======================================================================================
	// 見ているエンティティから値を取る
	//--------------------------------------------------------------------------------------
	// 持っていないコンポーネントは読まない。
	// RefData は持っていなくても非nullを返すので、必ず HasComponent で確かめること
	//======================================================================================
	bool UIGauge::FetchValue(Engine::ECS::World* a_pWorld)
	{
		if (a_pWorld == nullptr) return false;
		if (m_targetEntity == Engine::ECS::Limits::INVALID_ENTITY) return false;
		if (!a_pWorld->IsAliveEntity(m_targetEntity)) return false;

		switch (m_source)
		{
		case EGaugeSource::Health:
		{
			if (!a_pWorld->HasComponent<HealthComponent>(m_targetEntity)) return false;

			const auto* _pHealth = a_pWorld->RefData<HealthComponent>(m_targetEntity);
			if (!_pHealth) return false;

			m_current = _pHealth->currentHealth;
			m_max = _pHealth->maxHealth;
			return true;
		}

		case EGaugeSource::BoostFuel:
		{
			if (!a_pWorld->HasComponent<BoostComponent>(m_targetEntity)) return false;

			const auto* _pBoost = a_pWorld->RefData<BoostComponent>(m_targetEntity);
			if (!_pBoost) return false;

			m_current = _pBoost->currentFuel;
			m_max = _pBoost->maxFuel;
			return true;
		}

		case EGaugeSource::Overheat:
		{
			if (!a_pWorld->HasComponent<GunStateComponent>(m_targetEntity)) return false;

			const auto* _pGun = a_pWorld->RefData<GunStateComponent>(m_targetEntity);
			if (!_pGun) return false;

			// 熱は「溜まるほど満タン」。色のしきい値も溜まった側で読むことになるので、
			// 危険色は残量の大きい側へ置くこと
			m_current = _pGun->heat;
			m_max = _pGun->heatLimit;
			return true;
		}

		case EGaugeSource::ChargeDash:
		{
			if (!a_pWorld->HasComponent<ChargeDashComponent>(m_targetEntity)) return false;

			const auto* _pDash = a_pWorld->RefData<ChargeDashComponent>(m_targetEntity);
			if (!_pDash) return false;

			m_current = _pDash->chargeTimer;
			m_max = _pDash->chargeTime;
			return true;
		}

		case EGaugeSource::Manual:
		default:
			return true;
		}
	}

	//======================================================================================
	// 操作しているプレイヤーを探す
	//======================================================================================
	Engine::ECS::Entity UIGauge::FindPlayer(Engine::ECS::World* a_pWorld)
	{
		Engine::ECS::Entity _player = Engine::ECS::Limits::INVALID_ENTITY;
		if (a_pWorld == nullptr) return _player;

		a_pWorld->ForEach<const ActiveTag, const PlayerControllTag>(
			[&](
				Engine::ECS::ArchetypeChunk* a_pChunk,
				uint32_t a_count,
				const ActiveTag* a_activeTagArray,
				const PlayerControllTag* a_playerTagArray
			)
			{
				// 操作しているプレイヤーは1体の想定。先に見つかったものを使う
				if (_player != Engine::ECS::Limits::INVALID_ENTITY || a_count == 0) return;
				_player = a_pChunk->entityData[0];
			}
		);

		return _player;
	}

	//======================================================================================
	// プレイヤーの武器を引く
	//--------------------------------------------------------------------------------------
	// スロットが指すのは武器のエンティティ。撃てるかどうかや熱は武器側が持っているので、
	// オーバーヒートを出したいときはここまで辿る必要がある
	//======================================================================================
	Engine::ECS::Entity UIGauge::FindPlayerWeapon(Engine::ECS::World* a_pWorld, bool a_isRight)
	{
		if (a_pWorld == nullptr) return Engine::ECS::Limits::INVALID_ENTITY;

		const Engine::ECS::Entity _player = FindPlayer(a_pWorld);
		if (_player == Engine::ECS::Limits::INVALID_ENTITY) return Engine::ECS::Limits::INVALID_ENTITY;

		if (!a_pWorld->HasComponent<AttachmentSlotsComponent>(_player))
		{
			return Engine::ECS::Limits::INVALID_ENTITY;
		}

		const auto* _pSlots = a_pWorld->RefData<AttachmentSlotsComponent>(_player);
		if (!_pSlots) return Engine::ECS::Limits::INVALID_ENTITY;

		return a_isRight ? _pSlots->rightWeapon.id : _pSlots->leftWeapon.id;
	}

	//======================================================================================
	// 中身のピボットを合わせる
	//======================================================================================
	void UIGauge::ApplyFillPivot()
	{
		const int _fillIndex = FindDecorationIndex(m_fillDecorationName);
		if (_fillIndex < 0) return;

		// 動かさない場所を固定する。
		// 横幅に残量を掛けるだけなので、ピボットを置いた場所がそのまま
		// 「減っても動かない点」になる
		float _pivotX = 0.0f;
		switch (m_anchor)
		{
		case EGaugeAnchor::Right:  _pivotX = 1.0f; break;	// 右端を固定 : 左から減る
		case EGaugeAnchor::Center: _pivotX = 0.5f; break;	// 中央を固定 : 両側から減る

		case EGaugeAnchor::Left:
		default:                   _pivotX = 0.0f; break;	// 左端を固定 : 右から減る
		}

		m_decorationVec[_fillIndex].pivot.x = _pivotX;
	}

	//======================================================================================
	// 数値を流し込む
	//--------------------------------------------------------------------------------------
	// 文字は毎フレーム組み直すと、値が変わっていないのに文字列を作り直すことになる。
	// 変わったときだけ書き換える
	//======================================================================================
	void UIGauge::ApplyValueText()
	{
		if (m_textFormat == EGaugeTextFormat::None) return;

		const int _textIndex = FindDecorationIndex(m_textDecorationName);
		if (_textIndex < 0) return;

		Decoration::Decoration& _decoration = m_decorationVec[_textIndex];
		if (_decoration.type != Decoration::EDecorationType::Text) return;

		const std::string _text = MakeValueText();
		if (_text == m_appliedText && _decoration.text == _text) return;

		_decoration.text = _text;
		m_appliedText = _text;
	}

	//======================================================================================
	// 出す文字列を作る
	//======================================================================================
	std::string UIGauge::MakeValueText() const
	{
		const int _decimals = std::clamp(m_decimals, 0, 4);

		char _format[16] = {};
		std::snprintf(_format, sizeof(_format), "%%.%df", _decimals);

		char _current[32] = {};
		std::snprintf(_current, sizeof(_current), _format, m_current);

		switch (m_textFormat)
		{
		case EGaugeTextFormat::ValueAndMax:
		{
			char _max[32] = {};
			std::snprintf(_max, sizeof(_max), _format, m_max);

			return std::string(_current) + "/" + _max;
		}

		case EGaugeTextFormat::Percent:
		{
			char _percent[32] = {};
			std::snprintf(_percent, sizeof(_percent), _format, GetRatio() * 100.0f);

			return std::string(_percent) + "%";
		}

		case EGaugeTextFormat::Value:
		default:
			return _current;
		}
	}

	//======================================================================================
	// 残量に対応する色
	//--------------------------------------------------------------------------------------
	// 並びは残量の小さい順。両端より外は端の色をそのまま使う。
	//======================================================================================
	Math::Color UIGauge::CalcGaugeColor(float a_ratio) const
	{
		if (m_colorStopVec.empty()) return Engine::Color::WHITE;

		// 端より外
		if (a_ratio <= m_colorStopVec.front().ratio) return m_colorStopVec.front().color;
		if (a_ratio >= m_colorStopVec.back().ratio)  return m_colorStopVec.back().color;

		for (size_t _i = 0; _i + 1 < m_colorStopVec.size(); ++_i)
		{
			const GaugeColorStop& _low = m_colorStopVec[_i];
			const GaugeColorStop& _high = m_colorStopVec[_i + 1];

			if (a_ratio < _low.ratio || a_ratio >= _high.ratio) continue;

			// しきい値で切り替えるなら、下側の色をそのまま使う
			if (!m_isBlendColor) return _low.color;

			const float _width = _high.ratio - _low.ratio;
			if (_width <= 1e-6f) return _low.color;

			return Math::Color::Lerp(_low.color, _high.color, (a_ratio - _low.ratio) / _width);
		}

		return m_colorStopVec.back().color;
	}

	//======================================================================================
	// 描画
	//--------------------------------------------------------------------------------------
	// 飾りを配列順に回して、中身の1つだけ差し替えを掛ける。
	// まとめて描く DrawDecorations を使わないのは、重なり順(配列順)を保ったまま
	// 1つだけ別扱いにしたいため
	//======================================================================================
	void UIGauge::Draw(Engine::GameObject::ObjectContext& a_context)
	{
		if (!m_isVisible) return;

		// 値が取れていないフレームは何も出さない。
		// Visible を落とさないのは、進行役が握っている出し入れと取り合わないため
		if (m_isHideWhenNoValue && !m_hasValue) return;

		const float _ratio = GetRatio();
		const int _fillIndex = FindDecorationIndex(m_fillDecorationName);

		// 中身へ掛ける差し替え : 横だけ縮めて、残量の色を乗せる
		Decoration::DrawOverride _fillOverride = {};
		_fillOverride.sizeScale = { _ratio, 1.0f };
		_fillOverride.tint = CalcGaugeColor(_ratio);

		for (size_t _i = 0; _i < m_decorationVec.size(); ++_i)
		{
			if (static_cast<int>(_i) == _fillIndex)
			{
				// 空のときは幅0なので、描画側が弾いて何も出ない
				DrawDecorationAt(a_context, _i, _fillOverride);
				continue;
			}

			DrawDecorationAt(a_context, _i);
		}
	}

	//======================================================================================
	// シリアライズ
	//======================================================================================
	void UIGauge::Archive(Engine::Persistence::Archive& a_ar, Engine::GameObject::ObjectContext& a_context)
	{
		// 位置・色・飾りなどの共通ぶん
		UIBase::Archive(a_ar, a_context);

		a_ar.StringField("FillDecorationName", m_fillDecorationName);
		a_ar.Field("Anchor", m_anchor);

		//----------------------------------------------------------------------
		// 残量ごとの色
		//----------------------------------------------------------------------
		size_t _stopCount = m_colorStopVec.size();
		if (a_ar.BeginArray("ColorStops", _stopCount))
		{
			m_colorStopVec.resize(_stopCount);

			for (size_t _i = 0; _i < _stopCount; ++_i)
			{
				if (!a_ar.BeginObject(_i)) continue;

				a_ar.Field("ratio", m_colorStopVec[_i].ratio);
				a_ar.Field("color", m_colorStopVec[_i].color);

				a_ar.EndObject();
			}
			a_ar.EndArray();
		}

		a_ar.Field("IsBlendColor", m_isBlendColor);

		//----------------------------------------------------------------------
		// 数値
		//----------------------------------------------------------------------
		a_ar.StringField("TextDecorationName", m_textDecorationName);
		a_ar.Field("TextFormat", m_textFormat);
		a_ar.Field("Decimals", m_decimals);

		//----------------------------------------------------------------------
		// どこから値を取るか
		//----------------------------------------------------------------------
		a_ar.Field("Target", m_target);
		a_ar.Field("Source", m_source);
		a_ar.Field("IsHideWhenNoValue", m_isHideWhenNoValue);

		if (a_ar.IsLoading())
		{
			// 流し込み直させる
			m_appliedText.clear();
		}
	}

	//======================================================================================
	// インスペクター
	//======================================================================================
	void UIGauge::DrawInspector(Engine::GameObject::ObjectContext& a_context)
	{
		UIBase::DrawInspector(a_context);

		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();

		//----------------------------------------------------------------------
		// どこから値を取るか
		//----------------------------------------------------------------------
		ImGui::SeparatorText("Source");

		Engine::Editor::EditorHelper::DrawEnumCombo("Target", m_target);
		ImGui::TextDisabled("見るエンティティの決め方");

		Engine::Editor::EditorHelper::DrawEnumCombo("Source", m_source);
		ImGui::TextDisabled("見るコンポーネント。持っていなければ何も出ない");

		if (m_source != EGaugeSource::Manual)
		{
			ImGui::Checkbox("HideWhenNoValue", &m_isHideWhenNoValue);
			ImGui::SetItemTooltip("値が取れないフレームは描かない(ロックしていない等)");

			// 今どれを見ているかが分かるようにしておく
			if (m_targetEntity == Engine::ECS::Limits::INVALID_ENTITY)
			{
				ImGui::TextDisabled("Entity : none");
			}
			else
			{
				ImGui::Text("Entity : %u  (%s)",
					static_cast<uint32_t>(m_targetEntity),
					m_hasValue ? "ok" : "コンポーネントなし");
			}
		}

		ImGui::Spacing();
		ImGui::SeparatorText("Gauge");

		//----------------------------------------------------------------------
		// 中身
		//----------------------------------------------------------------------
		ImGui::InputText("FillDecoration", &m_fillDecorationName);
		ImGui::TextDisabled("横幅を縮める飾りの名前");

		// 指している飾りが本当にあるか、その場で分かるようにしておく
		if (FindDecorationIndex(m_fillDecorationName) < 0)
		{
			ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "その名前の飾りがありません");
		}

		Engine::Editor::EditorHelper::DrawEnumCombo("Anchor", m_anchor);
		ImGui::TextDisabled("減っても動かない場所。Center は両側から均等に減る");

		//----------------------------------------------------------------------
		// 色
		//----------------------------------------------------------------------
		ImGui::Spacing();
		ImGui::SeparatorText("Color");

		ImGui::Checkbox("BlendColor", &m_isBlendColor);
		ImGui::SetItemTooltip("切ると、しきい値でパッと切り替わる");

		int _removeIndex = -1;

		for (size_t _i = 0; _i < m_colorStopVec.size(); ++_i)
		{
			ImGui::PushID(static_cast<int>(_i));

			// ボタンを先に置く : 後ろへ並べると幅を取られて押しにくい
			if (Engine::Editor::EditorHelper::DeleteSmallButton("X")) _removeIndex = static_cast<int>(_i);

			ImGui::SameLine();
			ImGui::SetNextItemWidth(80.0f);
			if (ImGui::DragFloat("##ratio", &m_colorStopVec[_i].ratio, 0.01f, 0.0f, 1.0f))
			{
				m_colorStopVec[_i].ratio = std::clamp(m_colorStopVec[_i].ratio, 0.0f, 1.0f);
			}

			ImGui::SameLine();
			Engine::Editor::EditorHelper::DrawColorEdit("##color", m_colorStopVec[_i].color);

			ImGui::PopID();
		}

		if (_removeIndex >= 0) m_colorStopVec.erase(m_colorStopVec.begin() + _removeIndex);

		if (Engine::Editor::EditorHelper::CreateButton("Add Color Stop"))
		{
			m_colorStopVec.push_back({});
		}

		ImGui::SameLine();
		if (ImGui::Button("Sort"))
		{
			// 残量の小さい順に並んでいることが前提の作りなので、ここで直せるようにしておく
			std::sort(m_colorStopVec.begin(), m_colorStopVec.end(),
				[](const GaugeColorStop& a, const GaugeColorStop& b) { return a.ratio < b.ratio; });
		}
		ImGui::TextDisabled("残量の小さい順に並べること(Sort で整う)");

		//----------------------------------------------------------------------
		// 数値
		//----------------------------------------------------------------------
		ImGui::Spacing();
		ImGui::SeparatorText("Value Text");

		Engine::Editor::EditorHelper::DrawEnumCombo("TextFormat", m_textFormat);

		if (m_textFormat != EGaugeTextFormat::None)
		{
			ImGui::InputText("TextDecoration", &m_textDecorationName);
			ImGui::TextDisabled("数値を流し込む Text 飾りの名前。置き場所はその飾りの OffsetPos");

			const int _textIndex = FindDecorationIndex(m_textDecorationName);
			if (_textIndex < 0)
			{
				ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "その名前の飾りがありません");
			}
			else if (m_decorationVec[_textIndex].type != Decoration::EDecorationType::Text)
			{
				ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.3f, 1.0f), "その飾りが Text ではありません");
			}

			if (ImGui::DragInt("Decimals", &m_decimals, 1, 0, 4))
			{
				m_decimals = std::clamp(m_decimals, 0, 4);
				m_appliedText.clear();	// 桁を変えたらすぐ出し直す
			}
		}

		//----------------------------------------------------------------------
		// 値 : 実行中は入れる側が毎フレーム書き換える。
		//      ここで動かせるのは見た目を詰めるため
		//----------------------------------------------------------------------
		ImGui::Spacing();
		ImGui::SeparatorText("Value");

		if (m_source == EGaugeSource::Manual)
		{
			ImGui::TextDisabled("実行中は SetValue を呼ぶ側の値で上書きされる");
		}
		else
		{
			ImGui::TextDisabled("実行中は見ているコンポーネントの値で毎フレーム上書きされる");
		}

		ImGui::DragFloat("Max", &m_max, 1.0f, 0.0f, 100000.0f);
		ImGui::SliderFloat("Current", &m_current, 0.0f, std::max(m_max, 1.0f));

		ImGui::Text("Ratio : %.0f %%", GetRatio() * 100.0f);
	}
}
