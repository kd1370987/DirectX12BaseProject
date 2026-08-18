#include "UIButton.h"

#include "Engine/ECS/Internal/SystemContext.h"	// ObjectContext が運ぶサービス群
#include "Engine/MainEngine.h"
#include "Engine/Graphics/GraphicEngine.h"
#include "Engine/Input/InputManager/InputManager.h"
#include "Engine/Option/OptionManager.h"
#include "Engine/Window/NativeWindow.h"

#include "Engine/Editor/Helper/EditorHelper.h"

//==========================================================================================
// UIButton
//
// 「押されたことを知らせる」だけのUI。何をするかは SetOnClick で外から差し込む。
//
// ・当たり判定は描画と同じ矩形から作る
//     UIBase の位置・サイズ・ピボット・回転をそのまま使い、GraphicsEngine::PushUIData が
//     クアッドを組み立てるのと同じ式で軸を作る。判定用に別の値を持たせると、
//     エディターで見た目を動かしたときに「絵はここなのに押せない」ズレが必ず出る。
//
// ・座標系
//     UIのピクセル座標は「描画解像度(WindowOption)」基準。一方カーソルはクライアント領域の
//     ピクセルで取れる。バックバッファは描画解像度で作られてクライアント領域へ引き伸ばされる
//     ので、比率を掛けて描画解像度側へ直してから判定する。
//     (ウィンドウサイズを変えても判定がずれないようにするため、毎フレーム実測する)
//
// ・押下の作法
//     押し始めと離しの両方が矩形の内側にあるときだけ成立させる。押したまま外へ逃がせば
//     取り消せる、よくあるボタンと同じ操作感にするため。
//==========================================================================================
namespace App::Object
{
	void UIButton::Update(Engine::GameObject::ObjectContext& a_context)
	{
		// 「このフレームに押し切られたか」は毎フレーム作り直す
		m_isClicked = false;

		if (!a_context.pServices || !a_context.pServices->pInputManager)
		{
			m_isHovered = false;
			m_isPressed = false;
			m_isPressStartedInside = false;
			return;
		}

		auto& _input = *a_context.pServices->pInputManager;

		//==================================================================
		// 押せない状態
		//------------------------------------------------------------------
		// ・無効にされている
		// ・プレイモードでない / エディターで文字を打っている
		// このときは状態を全部落とす。押しっぱなしのまま無効にされて、
		// 有効へ戻した瞬間に押し切られたことにならないようにする。
		//==================================================================
		if (!m_isInteractable || !_input.IsGameInputEnable())
		{
			m_isHovered = false;
			m_isPressed = false;
			m_isPressStartedInside = false;
			return;
		}

		//==================================================================
		// カーソルが乗っているか
		//==================================================================
		Math::Vector2 _cursorPos = {};
		const bool _hasCursor = CalcCursorUIPos(a_context, _cursorPos);

		m_isHovered = _hasCursor && IsPointInside(_cursorPos);

		//==================================================================
		// 押下の進行
		//==================================================================
		const bool _isPressMoment   = _input.IsPress(m_clickActionName);	// 押した瞬間
		const bool _isHoldMoment    = _input.IsHold(m_clickActionName);		// 押している間
		const bool _isReleaseMoment = _input.IsRelease(m_clickActionName);	// 離した瞬間

		// 内側で押し始めたときだけ受け付ける
		if (_isPressMoment && m_isHovered)
		{
			m_isPressStartedInside = true;
		}

		m_isPressed = m_isPressStartedInside && _isHoldMoment;

		if (_isReleaseMoment)
		{
			// 押し始めと離しの両方が内側なら成立
			if (m_isPressStartedInside && m_isHovered)
			{
				m_isClicked = true;

				if (m_onClick) m_onClick();
			}

			m_isPressStartedInside = false;
			m_isPressed = false;
		}

		// 押していないのに押し始めの記録が残っていたら落とす
		// (ボタンを離した瞬間を取りこぼした場合の保険)
		if (!_isHoldMoment && !_isReleaseMoment)
		{
			m_isPressStartedInside = false;
			m_isPressed = false;
		}
	}

	void UIButton::Draw(Engine::GameObject::ObjectContext& a_context)
	{
		if (!a_context.pServices || !a_context.pServices->pMainEngine) return;

		auto* _pGE = a_context.pServices->pMainEngine->RefGraphicsEngine();
		if (!_pGE) return;

		// 状態ごとの色以外は UIBase と同じ。
		// UIBase::Draw を呼べないのは色だけ差し替えたいため
		_pGE->SubmitUI(
			m_texRef,
			m_pixelPos,
			m_pixelSize,
			GetStateColor(),
			m_rotation,
			m_layer,
			m_uvOffset,
			m_pivot
		);
	}

	//======================================================================================
	// 今の状態
	//======================================================================================
	EUIButtonState UIButton::GetState() const
	{
		if (!m_isInteractable) return EUIButtonState::Disabled;
		if (m_isPressed)       return EUIButtonState::Pressed;
		if (m_isHovered)       return EUIButtonState::Hovered;

		return EUIButtonState::Normal;
	}

	const Math::Color& UIButton::GetStateColor() const
	{
		switch (GetState())
		{
		case EUIButtonState::Disabled: return m_disableColor;
		case EUIButtonState::Pressed:  return m_pressColor;
		case EUIButtonState::Hovered:  return m_hoverColor;

		case EUIButtonState::Normal:
		default:
			// 通常時は UIBase の色。白テクスチャ1枚を色で作り分ける運用に合わせる
			return m_color;
		}
	}

	//======================================================================================
	// カーソル位置をUIのピクセル座標へ直す
	//--------------------------------------------------------------------------------------
	// クライアント領域(実際のウィンドウの大きさ) → 描画解像度(UIの座標系)。
	// バックバッファは描画解像度で作られ、クライアント領域へ引き伸ばして表示されるので、
	// 単純に比率を掛ければよい。
	//======================================================================================
	bool UIButton::CalcCursorUIPos(Engine::GameObject::ObjectContext& a_context, Math::Vector2& a_outPos) const
	{
		if (!a_context.pServices) return false;
		if (!a_context.pServices->pInputManager || !a_context.pServices->pOptionManager) return false;
		if (!a_context.pServices->pMainEngine) return false;

		// カーソル(クライアント座標)
		Math::Vector2 _clientPos = {};
		if (!a_context.pServices->pInputManager->GetCursorClientPos(_clientPos)) return false;

		// 描画解像度
		const auto& _winOp = a_context.pServices->pOptionManager->GetWindowOption();
		const float _renderW = static_cast<float>(_winOp.windowWidth);
		const float _renderH = static_cast<float>(_winOp.windowHeight);
		if (_renderW <= 0.0f || _renderH <= 0.0f) return false;

		// クライアント領域の実サイズ
		const auto* _pWind = a_context.pServices->pMainEngine->GetNativeWindow();
		if (!_pWind) return false;

		const float _clientW = static_cast<float>(_pWind->GetClientWidth());
		const float _clientH = static_cast<float>(_pWind->GetClientHeight());

		// 最小化中は0になる
		if (_clientW <= 0.0f || _clientH <= 0.0f) return false;

		a_outPos.x = _clientPos.x * (_renderW / _clientW);
		a_outPos.y = _clientPos.y * (_renderH / _clientH);

		return true;
	}

	//======================================================================================
	// 矩形の内側か
	//--------------------------------------------------------------------------------------
	// GraphicsEngine::PushUIData がクアッドを組み立てるのと同じ式で軸を作り、
	// その軸へ射影した長さで判定する。回転もピボットもそのまま効く。
	//
	//   ローカル+X(画面右) を回したもの : ( cos,  sin)
	//   ローカル+Y(画面上) を回したもの : ( sin, -cos)
	//   クアッド中心 : ピボット位置 + R * ピボットからのずれ
	//======================================================================================
	bool UIButton::IsPointInside(const Math::Vector2& a_uiPos) const
	{
		// 判定の半サイズ(余白ぶんを足す)
		const Math::Vector2 _half = {
			m_pixelSize.x * 0.5f + m_hitPadding.x,
			m_pixelSize.y * 0.5f + m_hitPadding.y
		};

		// 大きさが無ければ押しようがない
		if (_half.x <= 0.0f || _half.y <= 0.0f) return false;

		const float _rad = DirectX::XMConvertToRadians(m_rotation);
		const float _cos = std::cos(_rad);
		const float _sin = std::sin(_rad);

		// ピボットからクアッド中心までのずれ(回転前)
		const Math::Vector2 _pivotOff = {
			(0.5f - m_pivot.x) * m_pixelSize.x,
			(0.5f - m_pivot.y) * m_pixelSize.y
		};

		// 回転はピボットを中心に行われる
		const Math::Vector2 _center = {
			m_pixelPos.x + (_pivotOff.x * _cos - _pivotOff.y * _sin),
			m_pixelPos.y + (_pivotOff.x * _sin + _pivotOff.y * _cos)
		};

		const Math::Vector2 _diff = { a_uiPos.x - _center.x, a_uiPos.y - _center.y };

		// 各軸へ射影した長さ(軸はどちらも単位ベクトル)
		const float _u = _diff.x * _cos + _diff.y * _sin;
		const float _v = _diff.x * _sin - _diff.y * _cos;

		return (std::fabs(_u) <= _half.x) && (std::fabs(_v) <= _half.y);
	}

	//======================================================================================
	// シリアライズ
	//======================================================================================
	void UIButton::Archive(Engine::Persistence::Archive& a_ar, Engine::GameObject::ObjectContext& a_context)
	{
		// テクスチャ・色・位置などの共通ぶん
		UIBase::Archive(a_ar, a_context);

		a_ar.StringField("ClickActionName", m_clickActionName);

		a_ar.Field("HoverColor", m_hoverColor);
		a_ar.Field("PressColor", m_pressColor);
		a_ar.Field("DisableColor", m_disableColor);

		a_ar.Field("HitPadding", m_hitPadding);
		a_ar.Field("IsInteractable", m_isInteractable);
	}

	//======================================================================================
	// インスペクター
	//======================================================================================
	void UIButton::DrawInspector(Engine::GameObject::ObjectContext& a_context)
	{
		UIBase::DrawInspector(a_context);

		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();

		ImGui::SeparatorText("Button");

		ImGui::Checkbox("Interactable", &m_isInteractable);
		ImGui::SameLine();
		ImGui::TextDisabled("(切ると押せなくなる)");

		// 押下に使う入力アクション名
		ImGui::InputText("ClickAction", &m_clickActionName);
		ImGui::TextDisabled("InputManager へ登録したアクション名");

		ImGui::Spacing();

		// 状態ごとの色。通常時の色は UIBase 側の Color
		ImGui::SeparatorText("State Color");
		ImGui::TextDisabled("Normal は上の Color を使う");
		Engine::Editor::EditorHelper::DrawColorEdit("Hover", m_hoverColor);
		Engine::Editor::EditorHelper::DrawColorEdit("Press", m_pressColor);
		Engine::Editor::EditorHelper::DrawColorEdit("Disable", m_disableColor);

		ImGui::Spacing();

		// 当たり判定
		ImGui::SeparatorText("Hit");
		ImGui::DragFloat2("HitPadding", &m_hitPadding.x, 1.0f);
		ImGui::TextDisabled("見た目の矩形へ足す余白(px)");

		// 実行中の状態は毎フレーム上書きされるので表示のみ
		ImGui::Spacing();
		ImGui::SeparatorText("Runtime");

		static const char* _stateName[] = { "Normal", "Hovered", "Pressed", "Disabled" };
		ImGui::Text("State   : %s", _stateName[static_cast<int>(GetState())]);
		ImGui::Text("Hovered : %s", m_isHovered ? "yes" : "no");
		ImGui::Text("Pressed : %s", m_isPressed ? "yes" : "no");
		ImGui::Text("OnClick : %s", m_onClick ? "set" : "none");
	}
}
