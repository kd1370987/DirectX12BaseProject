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
		// ・出していない(見えていないものは押せない)
		// ・無効にされている
		// ・プレイモードでない / エディターで文字を打っている
		// このときは状態を全部落とす。押しっぱなしのまま無効にされて、
		// 有効へ戻した瞬間に押し切られたことにならないようにする。
		//==================================================================
		if (!m_isVisible || !m_isInteractable || !_input.IsGameInputEnable())
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

		m_isHovered = _hasCursor && IsPointInsideSelf(_cursorPos);

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
		// 出さない指示が出ているものは描かない(UIBase::Draw と同じ扱い)
		if (!m_isVisible) return;

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
	// 矩形の内側か
	//--------------------------------------------------------------------------------------
	// 判定そのものは UIBase の共通実装(描画と同じ式でクアッドを組む)。
	// ここは自分の位置・大きさ・ピボット・回転をそのまま渡すだけにしてある。
	// オブジェクトを置かずに並べたUI(ステージ一覧など)からも同じ判定を使うため。
	//======================================================================================
	bool UIButton::IsPointInsideSelf(const Math::Vector2& a_uiPos) const
	{
		return UIBase::IsPointInside(
			a_uiPos,
			m_pixelPos,
			m_pixelSize,
			m_pivot,
			m_rotation,
			m_hitPadding);
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
