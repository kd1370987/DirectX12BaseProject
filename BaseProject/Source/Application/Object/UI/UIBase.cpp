#include "UIBase.h"

#include "Engine/ECS/Internal/SystemContext.h"	// ObjectContext が運ぶサービス群
#include "Engine/MainEngine.h"
#include "Engine/Graphics/GraphicEngine.h"
#include "Engine/Resource/Manager/ResourceManager/ResourceManager.h"
#include "Engine/Resource/Manager/AssetDatabase/AssetDatabase.h"
#include "Engine/Common/Color.h"
#include "Engine/Option/OptionManager.h"	// ウィンドウ解像度(px)取得用
#include "Engine/Input/InputManager/InputManager.h"	// カーソル位置の取得用
#include "Engine/Window/NativeWindow.h"				// クライアント領域の実サイズ取得用

#include "../../../Engine/Editor/Helper/EditorHelper.h"

namespace App::Object
{
	void UIBase::Release(Engine::GameObject::ObjectContext& a_context)
	{}

	void UIBase::Draw(Engine::GameObject::ObjectContext& a_context)
	{
		// 出さない指示が出ているものは描かない
		if (!m_isVisible) return;

		if (!a_context.pServices || !a_context.pServices->pMainEngine) return;

		auto* _pGE = a_context.pServices->pMainEngine->RefGraphicsEngine();
		if (!_pGE) return;

		// 座標・サイズ(px)、回転(度)、正規化ピボット[0,1]をそのまま渡す。
		// NDC変換・アスペクト補正・回転・ピボット処理はエンジン側(SubmitUI)が行う。
		_pGE->SubmitUI(
			m_texRef,
			m_pixelPos,
			m_pixelSize,
			m_color,
			m_rotation,
			m_layer,
			m_uvOffset,
			m_pivot
		);
	}
	void UIBase::Archive(Engine::Persistence::Archive& a_ar, Engine::GameObject::ObjectContext& a_context)
	{
		a_ar.GUIDField("TexGUID", m_texGUID);
		a_ar.Field("Color", m_color);

		a_ar.Field("PosPixel", m_pixelPos);
		a_ar.Field("SizePixel", m_pixelSize);
		a_ar.Field("m_rotation",m_rotation);
		a_ar.Field("m_pivot",m_pivot);
		a_ar.Field("m_uvOffset",m_uvOffset);
		a_ar.Field("m_layer", m_layer);
		a_ar.Field("m_scale", m_scale);

		// 出し分けの状態。※ 追加は必ずここより上でなく末尾へ
		//    (バイナリは並び順で読むので、間に挟むと既存のデータがずれる)
		a_ar.Field("IsVisible", m_isVisible);

		m_editSize = m_pixelSize;

		// 読み込み時は復元したGUIDでテクスチャを引き直す。
		// 実体が届くのを待つ必要はないので、要求だけ出して先へ進む。
		// 描画側は IsReady を見て、まだのフレームは描かない
		if (a_ar.IsLoading())
		{
			if (!m_texGUID.IsValid()) return;
			if (!a_context.pServices || !a_context.pServices->pResourceManager) return;

			m_texRef = a_context.pServices->pResourceManager->RequestLoad<Engine::Resource::Texture>(m_texGUID);
		}
	}
	//======================================================================================
	// カーソル位置をUIのピクセル座標へ直す
	//--------------------------------------------------------------------------------------
	// クライアント領域(実際のウィンドウの大きさ) → 描画解像度(UIの座標系)。
	// バックバッファは描画解像度で作られ、クライアント領域へ引き伸ばして表示されるので、
	// 単純に比率を掛ければよい。
	// (ウィンドウサイズを変えても判定がずれないよう、毎フレーム実測する)
	//======================================================================================
	bool UIBase::CalcCursorUIPos(Engine::GameObject::ObjectContext& a_context, Math::Vector2& a_outPos)
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
	// (判定用に別の値を持たせると「絵はここなのに押せない」ズレが必ず出るため、
	//  描画に渡すものと同じ値をそのまま受け取る形にしてある)
	//
	//   ローカル+X(画面右) を回したもの : ( cos,  sin)
	//   ローカル+Y(画面上) を回したもの : ( sin, -cos)
	//   クアッド中心 : ピボット位置 + R * ピボットからのずれ
	//======================================================================================
	bool UIBase::IsPointInside(
		const Math::Vector2& a_uiPos,
		const Math::Vector2& a_pixelPos,
		const Math::Vector2& a_pixelSize,
		const Math::Vector2& a_pivot,
		float a_rotationDeg,
		const Math::Vector2& a_hitPadding)
	{
		// 判定の半サイズ(余白ぶんを足す)
		const Math::Vector2 _half = {
			a_pixelSize.x * 0.5f + a_hitPadding.x,
			a_pixelSize.y * 0.5f + a_hitPadding.y
		};

		// 大きさが無ければ触りようがない
		if (_half.x <= 0.0f || _half.y <= 0.0f) return false;

		const float _rad = DirectX::XMConvertToRadians(a_rotationDeg);
		const float _cos = std::cos(_rad);
		const float _sin = std::sin(_rad);

		// ピボットからクアッド中心までのずれ(回転前)
		const Math::Vector2 _pivotOff = {
			(0.5f - a_pivot.x) * a_pixelSize.x,
			(0.5f - a_pivot.y) * a_pixelSize.y
		};

		// 回転はピボットを中心に行われる
		const Math::Vector2 _center = {
			a_pixelPos.x + (_pivotOff.x * _cos - _pivotOff.y * _sin),
			a_pixelPos.y + (_pivotOff.x * _sin + _pivotOff.y * _cos)
		};

		const Math::Vector2 _diff = { a_uiPos.x - _center.x, a_uiPos.y - _center.y };

		// 各軸へ射影した長さ(軸はどちらも単位ベクトル)
		const float _u = _diff.x * _cos + _diff.y * _sin;
		const float _v = _diff.x * _sin - _diff.y * _cos;

		return (std::fabs(_u) <= _half.x) && (std::fabs(_v) <= _half.y);
	}

	void UIBase::DrawInspector(Engine::GameObject::ObjectContext& a_context)
	{
		if (!a_context.pServices) return;
		if (!a_context.pServices->pOptionManager || !a_context.pServices->pResourceManager) return;

		// ウィンドウサイズの取得
		const auto& _winOp = a_context.pServices->pOptionManager->GetWindowOption();
		const float _w = static_cast<float>(_winOp.windowWidth);
		const float _h = static_cast<float>(_winOp.windowHeight);

		// 表示するか : 出し分けを持つ画面(ホームなど)は進行役がここを切り替える
		ImGui::Checkbox("Visible", &m_isVisible);
		ImGui::SameLine();
		ImGui::TextDisabled("(切ると描画も入力も止まる)");

		// テクスチャの編集
		if (ImGui::CollapsingHeader("Texture"))
		{
			// テクスチャ選択
			if (Engine::Editor::EditorHelper::DrawAssetSelectComboGUID(
				"Change Texture",
				"Texture",
				m_texGUID))
			{
				// テクスチャの差し替え : 届くまでは描画側がスキップする
				m_texRef = a_context.pServices->pResourceManager->RequestLoad<Engine::Resource::Texture>(m_texGUID);
			}
			Engine::Editor::EditorHelper::DrawTexture(m_texRef, 256, 256);
		}

		ImGui::Spacing();

		// 色 : テクスチャに掛ける乗算色。畳まずに常に出しておく
		// (白いテクスチャを1枚用意して、色だけで作り分けられるようにするため)
		Engine::Editor::EditorHelper::DrawColorEdit("Color", m_color);

		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();

		// 座標系
		ImGui::DragFloat2("PixelPos",&m_pixelPos.x,1.0f);						// スクリーン座標
		ImGui::Spacing();

		ImGui::DragFloat("Rotation", &m_rotation, 0.1f, -360.0f, 360.0f);
		if (m_rotation >= 360) m_rotation -= 360;
		if (m_rotation <= -360) m_rotation += 360;

		ImGui::Spacing();
		if (ImGui::DragFloat("Scale", &m_scale, 0.01f, 0.0f))						// 等倍拡縮
		{
			m_pixelSize = m_editSize * m_scale;
		}
		if (ImGui::DragFloat2("PixelSize", &m_pixelSize.x, 1.0f, 0.0f, 8192.0f))	// ピクセルサイズ
		{
			m_editSize = m_pixelSize / m_scale;
		}
		ImGui::Spacing();

		// 初期化用ボタン
		if (ImGui::Button("RefreshTransform"))
		{
			m_pixelPos = { _w / 2.0f,_h / 2.0f };

			const auto* _pTex = a_context.pServices->pResourceManager->Get(m_texRef);
			if (_pTex)
			{
				const auto& _desc = _pTex->GetDesc();
				m_pixelSize.x = _desc.Width;
				m_pixelSize.y = _desc.Height;
			}
			else
			{
				m_pixelSize = { _w / 4 ,_h / 4 };
			}
			
			m_rotation = 0.0f;
		}

		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();

		// ピボット : 正規化[0,1]。(0.5,0.5)=中心, (0,0)=左上, (1,1)=右下。
		// この点が PixelPos に配置され、回転の中心にもなる。
		ImGui::DragFloat2("Pivot (0-1)", &m_pivot.x, 0.01f, 0.0f, 1.0f);
		ImGui::DragFloat2("UVOffset", &m_uvOffset.x, 0.01f);
		ImGui::DragFloat("LayerZ", &m_layer, 1.0f);
		
	}
	bool UIBase::DrawGizmo(const Engine::GameObject::ObjectGizmoContext& a_ctx, Engine::GameObject::ObjectContext& a_context)
	{
		// シーンビュー上にドラッグ可能なハンドルを出してピクセル座標を編集する
		if (a_ctx.viewportSize.x <= 0.0f || a_ctx.viewportSize.y <= 0.0f) return false;
		if (!a_context.pServices || !a_context.pServices->pOptionManager) return false;

		// ウィンドウサイズの取得
		const auto& _winOp = a_context.pServices->pOptionManager->GetWindowOption();
		const float _w = static_cast<float>(_winOp.windowWidth);
		const float _h = static_cast<float>(_winOp.windowHeight);
		if (_w <= 0.0f || _h <= 0.0f) return false;

		// ゲーム内ピクセル(左上原点) から シーンビュー上ピクセルへ。
		// m_pixelPos はピボットのスクリーン座標なので、ハンドルはそのままピボット位置を指す。
		ImVec2 _handle = {};
		_handle.x = a_ctx.viewportPos.x + (m_pixelPos.x / _w) * a_ctx.viewportSize.x;
		_handle.y = a_ctx.viewportPos.y + (m_pixelPos.y / _h) * a_ctx.viewportSize.y;

		// ギズモハンドルの半径 : ピクセル
		static const float _handleRadius = 9.0f;

		// ドラッグ操作用の透明ボタン
		ImGui::SetCursorScreenPos(ImVec2(_handle.x - _handleRadius,_handle.y - _handleRadius));
		ImGui::InvisibleButton("##UIGizmo", ImVec2(_handleRadius * 2.0f, _handleRadius * 2.0f));
		const bool _active = ImGui::IsItemActive();			// 選択されているかどうか
		const bool _hovered = ImGui::IsItemHovered();		// カーソルが重なっているかどうか

		// ハンドル描画(十字 + 円)
		ImDrawList* _dl = ImGui::GetWindowDrawList();
		const ImU32 _col = _active ? IM_COL32(255, 200, 0, 255)
			: _hovered ? IM_COL32(255, 255, 255, 255)
			: IM_COL32(0, 200, 255, 255);
		_dl->AddCircle(_handle, _handleRadius, _col, 20, 2.0f);
		_dl->AddLine(ImVec2(_handle.x - _handleRadius * 1.8f, _handle.y), ImVec2(_handle.x + _handleRadius * 1.8f, _handle.y), _col, 1.5f);
		_dl->AddLine(ImVec2(_handle.x, _handle.y - _handleRadius * 1.8f), ImVec2(_handle.x, _handle.y + _handleRadius * 1.8f), _col, 1.5f);

		// ドラッグ中はマウス位置からピクセル座標を逆算して更新
		if (_active)
		{
			const ImVec2 _mouse = ImGui::GetMousePos();
			const float _u = (_mouse.x - a_ctx.viewportPos.x) / a_ctx.viewportSize.x;	// 0..1
			const float _v = (_mouse.y - a_ctx.viewportPos.y) / a_ctx.viewportSize.y;	// 0..1
			m_pixelPos.x = std::clamp(_u * _w, 0.0f, _w);
			m_pixelPos.y = std::clamp(_v * _h, 0.0f, _h);
		}

		return true;
	}
}