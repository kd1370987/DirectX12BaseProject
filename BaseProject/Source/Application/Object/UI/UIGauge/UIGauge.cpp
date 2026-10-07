#include "UIGauge.h"

#include "Engine/ECS/System/SystemContext.h"	// ObjectContext が運ぶサービス群
#include "Application/ECS/World/APPWorld.h"
#include "Engine/EditorField/EditorField.h"

#include "Application/InstanceResource/PlayerHUDResource.h"

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
			m_hasValue = PickValue(a_context);
		}

		// 伸び縮みの向きへピボットを合わせる
		ApplyFillPivot();

		// 数値の流し込み
		ApplyValueText();
	}

	//======================================================================================
	// 値を取る
	//--------------------------------------------------------------------------------------
	// 見る相手も値も HUDGatherSystem が PlayerHUDResource へ集めてある。ここは引くだけ。
	// 毎フレーム引き直すので、ロックが外れた・敵が消えた・武器を持ち替えた のいずれにも追従する
	//======================================================================================
	namespace
	{
		// 見る相手 : Manual は相手を持たない(値は SetValue で入れる)
		bool ToHUDSubject(EGaugeTarget a_target, InstanceResource::EHUDSubject& a_outSubject)
		{
			switch (a_target)
			{
			case EGaugeTarget::Player:				a_outSubject = InstanceResource::EHUDSubject::Player;		return true;
			case EGaugeTarget::LockedEnemy:			a_outSubject = InstanceResource::EHUDSubject::LockedEnemy;	return true;
			case EGaugeTarget::PlayerRightWeapon:	a_outSubject = InstanceResource::EHUDSubject::RightWeapon;	return true;
			case EGaugeTarget::PlayerLeftWeapon:	a_outSubject = InstanceResource::EHUDSubject::LeftWeapon;	return true;

			case EGaugeTarget::Manual:
			default:								return false;
			}
		}

		// 見る値 : Manual はコンポーネントを見ない
		bool ToHUDGaugeKind(EGaugeSource a_source, InstanceResource::EHUDGaugeKind& a_outKind)
		{
			switch (a_source)
			{
			case EGaugeSource::Health:		a_outKind = InstanceResource::EHUDGaugeKind::Health;		return true;
			case EGaugeSource::BoostFuel:	a_outKind = InstanceResource::EHUDGaugeKind::BoostFuel;		return true;
			case EGaugeSource::Overheat:	a_outKind = InstanceResource::EHUDGaugeKind::Overheat;		return true;
			case EGaugeSource::ChargeDash:	a_outKind = InstanceResource::EHUDGaugeKind::ChargeDash;	return true;

			case EGaugeSource::Manual:
			default:						return false;
			}
		}
	}

	bool UIGauge::PickValue(Engine::GameObject::ObjectContext& a_context)
	{
		InstanceResource::EHUDSubject _subject = {};
		InstanceResource::EHUDGaugeKind _kind = {};
		if (!ToHUDSubject(m_target, _subject)) return false;
		if (!ToHUDGaugeKind(m_source, _kind)) return false;

		auto* _pWorld = a_context.pWorld;
		if (!_pWorld) return false;
		if (!_pWorld->HasResource<InstanceResource::PlayerHUDResource>()) return false;

		// 相手が居ない・そのコンポーネントを持っていないときは isValid が立っていない
		const auto& _hud = _pWorld->GetResource<InstanceResource::PlayerHUDResource>();
		const InstanceResource::HUDGaugeValue& _value = _hud.GetGauge(_subject, _kind);
		if (!_value.isValid) return false;

		m_current = _value.current;
		m_max = _value.max;
		return true;
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

		// ここから下は UIGauge のぶん。基底(UIBase)とは区切りを分けてあるので、
		// どちらに足しても互いの読み出しはずれない。足すときはこの区切りの末尾へ
		Engine::Persistence::ArchiveSection _section(a_ar, "UIGauge");

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

		//----------------------------------------------------------------------
		// どこから値を取るか
		//----------------------------------------------------------------------
		Engine::EditorField::Header("Source");

		Engine::EditorField::Field("Target", m_target);
		Engine::EditorField::Tooltip("見るエンティティの決め方");

		Engine::EditorField::Field("Source", m_source);
		Engine::EditorField::Tooltip("見るコンポーネント。持っていなければ何も出ない");

		if (m_source != EGaugeSource::Manual)
		{
			Engine::EditorField::Field("HideWhenNoValue", m_isHideWhenNoValue);
			Engine::EditorField::Tooltip("値が取れないフレームは描かない(ロックしていない等)");

			// 今取れているかが分かるようにしておく
			if (m_target == EGaugeTarget::Manual)
			{
				Engine::EditorField::HelpText("Target が Manual のときは値を取れない(Source も Manual にして SetValue で入れる)");
			}
			else
			{
				Engine::EditorField::Value("Value", "%s", m_hasValue ? "ok" : "none (相手が居ない/コンポーネントなし)");
			}
		}

		Engine::EditorField::Header("Gauge");

		//----------------------------------------------------------------------
		// 中身
		//----------------------------------------------------------------------
		Engine::EditorField::Field("FillDecoration", m_fillDecorationName);
		Engine::EditorField::Tooltip("横幅を縮める飾りの名前");

		// 指している飾りが本当にあるか、その場で分かるようにしておく
		if (FindDecorationIndex(m_fillDecorationName) < 0)
		{
			Engine::EditorField::ErrorText("その名前の飾りがありません");
		}

		Engine::EditorField::Field("Anchor", m_anchor);
		Engine::EditorField::Tooltip("減っても動かない場所。Center は両側から均等に減る");

		//----------------------------------------------------------------------
		// 色
		//----------------------------------------------------------------------
		Engine::EditorField::Header("Color");

		Engine::EditorField::Field("BlendColor", m_isBlendColor);
		Engine::EditorField::Tooltip("切ると、しきい値でパッと切り替わる");

		int _removeIndex = -1;

		for (size_t _i = 0; _i < m_colorStopVec.size(); ++_i)
		{
			Engine::EditorField::IDScope _id(static_cast<int>(_i));

			// ボタンを先に置く : 後ろへ並べると幅を取られて押しにくい
			if (Engine::EditorField::DeleteSmallButton("X")) _removeIndex = static_cast<int>(_i);

			Engine::EditorField::SameLine();
			Engine::EditorField::SetNextItemWidth(80.0f);
			if (Engine::EditorField::Field("##ratio", m_colorStopVec[_i].ratio, 0.01f, 0.0f, 1.0f))
			{
				m_colorStopVec[_i].ratio = std::clamp(m_colorStopVec[_i].ratio, 0.0f, 1.0f);
			}

			Engine::EditorField::SameLine();
			Engine::EditorField::Field("##color", m_colorStopVec[_i].color);
		}

		if (_removeIndex >= 0) m_colorStopVec.erase(m_colorStopVec.begin() + _removeIndex);

		if (Engine::EditorField::CreateButton("Add Color Stop"))
		{
			m_colorStopVec.push_back({});
		}

		Engine::EditorField::SameLine();
		if (Engine::EditorField::Button("Sort"))
		{
			// 残量の小さい順に並んでいることが前提の作りなので、ここで直せるようにしておく
			std::sort(m_colorStopVec.begin(), m_colorStopVec.end(),
				[](const GaugeColorStop& a, const GaugeColorStop& b) { return a.ratio < b.ratio; });
		}
		Engine::EditorField::Tooltip("残量の小さい順に並べること(Sort で整う)");

		//----------------------------------------------------------------------
		// 数値
		//----------------------------------------------------------------------
		Engine::EditorField::Header("Value Text");

		Engine::EditorField::Field("TextFormat", m_textFormat);

		if (m_textFormat != EGaugeTextFormat::None)
		{
			Engine::EditorField::Field("TextDecoration", m_textDecorationName);
			Engine::EditorField::Tooltip("数値を流し込む Text 飾りの名前。置き場所はその飾りの OffsetPos");

			const int _textIndex = FindDecorationIndex(m_textDecorationName);
			if (_textIndex < 0)
			{
				Engine::EditorField::ErrorText("その名前の飾りがありません");
			}
			else if (m_decorationVec[_textIndex].type != Decoration::EDecorationType::Text)
			{
				Engine::EditorField::WarningText("その飾りが Text ではありません");
			}

			if (Engine::EditorField::Field("Decimals", m_decimals, 1, 0, 4))
			{
				m_decimals = std::clamp(m_decimals, 0, 4);
				m_appliedText.clear();	// 桁を変えたらすぐ出し直す
			}
		}

		//----------------------------------------------------------------------
		// 値 : 実行中は入れる側が毎フレーム書き換える。
		//      ここで動かせるのは見た目を詰めるため
		//----------------------------------------------------------------------
		Engine::EditorField::Header("Value");

		if (m_source == EGaugeSource::Manual)
		{
			Engine::EditorField::HelpText("実行中は SetValue を呼ぶ側の値で上書きされる");
		}
		else
		{
			Engine::EditorField::HelpText("実行中は見ているコンポーネントの値で毎フレーム上書きされる");
		}

		Engine::EditorField::Field("Max", m_max, 1.0f, 0.0f, 100000.0f);
		Engine::EditorField::Slider("Current", m_current, 0.0f, std::max(m_max, 1.0f));

		Engine::EditorField::Value("Ratio", "%.0f %%", GetRatio() * 100.0f);
	}
}
