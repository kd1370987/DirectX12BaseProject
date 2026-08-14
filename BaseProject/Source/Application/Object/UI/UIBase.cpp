#include "UIBase.h"

#include "Engine/MainEngine.h"
#include "Engine/Graphics/GraphicEngine.h"
#include "Engine/Resource/Manager/ResourceManager/ResourceManager.h"
#include "Engine/Resource/Manager/AssetDatabase/AssetDatabase.h"
#include "Engine/Common/Color.h"
#include "Engine/Option/OptionManager.h"	// ウィンドウ解像度(px)取得用

#include "../../../Engine/Editor/Helper/EditorHelper.h"

namespace App::Object
{
	void UIBase::Release(Engine::GameObject::ObjectContext& a_context)
	{}

	void UIBase::Draw(Engine::GameObject::ObjectContext& a_context)
	{
		auto* _pGE = Engine::MainEngine::Instance().RefGraphicsEngine();
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
	void UIBase::Archive(Engine::Persistence::Archive& a_ar)
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

		m_editSize = m_pixelSize;

		// 読み込み時は復元したGUIDでテクスチャを引き直す。
		// 実体が届くのを待つ必要はないので、要求だけ出して先へ進む。
		// 描画側は IsReady を見て、まだのフレームは描かない
		if (a_ar.IsLoading())
		{
			if (!m_texGUID.IsValid()) return;
			m_texRef = Engine::Resource::ResourceManager::Instance().RequestLoad<Engine::Resource::Texture>(m_texGUID);
		}
	}
	void UIBase::DrawInspector()
	{
		// ウィンドウサイズの取得
		const auto& _winOp = Engine::Option::OptionManager::GetInstance().GetWindowOption();
		const float _w = static_cast<float>(_winOp.windowWidth);
		const float _h = static_cast<float>(_winOp.windowHeight);

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
				m_texRef = Engine::Resource::ResourceManager::Instance().RequestLoad<Engine::Resource::Texture>(m_texGUID);
			}
			ImGui::DragFloat4("ColorScale", &m_color.x, 0.01f, 0.0f);
			Engine::Editor::EditorHelper::DrawTexture(m_texRef, 256, 256);
		}

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

			const auto* _pTex = Engine::Resource::ResourceManager::Instance().Get(m_texRef);
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
	bool UIBase::DrawGizmo(const Engine::GameObject::ObjectGizmoContext& a_ctx)
	{
		// シーンビュー上にドラッグ可能なハンドルを出してピクセル座標を編集する
		if (a_ctx.viewportSize.x <= 0.0f || a_ctx.viewportSize.y <= 0.0f) return false;

		// ウィンドウサイズの取得
		const auto& _winOp = Engine::Option::OptionManager::GetInstance().GetWindowOption();
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