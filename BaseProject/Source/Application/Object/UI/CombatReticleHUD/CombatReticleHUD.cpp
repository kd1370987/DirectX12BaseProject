#include "CombatReticleHUD.h"

#include "Engine/MainEngine.h"
#include "Engine/Graphics/GraphicEngine.h"
#include "Engine/Resource/Manager/ResourceManager/ResourceManager.h"
#include "Engine/Resource/Manager/AssetDatabase/AssetDatabase.h"
#include "Engine/Common/Color.h"

namespace App::Object
{
	namespace
	{
		// テスト用レティクルテクスチャのパス
		constexpr const char* RETICLE_TEXTURE_PATH = "Asset/Texture/Test/uiTest.png";
	}

	void CombatReticleHUD::Init(Engine::GameObject::ObjectContext& a_context)
	{
		// パスからGUIDを引いてテクスチャを読み込む
		Engine::GUID _guid = Engine::Resource::AssetDatabase::Instance().GetGUIDFromFilePath(RETICLE_TEXTURE_PATH);
		m_reticleTexRef = Engine::Resource::ResourceManager::Instance().Load<Engine::Resource::Texture>(_guid);
	}

	void CombatReticleHUD::Update(Engine::GameObject::ObjectContext& a_context)
	{
		// テスト段階のため更新処理は無し
	}

	void CombatReticleHUD::Draw(Engine::GameObject::ObjectContext& a_context)
	{
		auto* _pGE = Engine::MainEngine::Instance().RefGraphicsEngine();
		if (!_pGE) return;

		// 画面中央にレティクルを描画命令として積む。
		// スクリーン矩形はNDC半径指定(ベースクアッドが-1〜1のため、size分だけ広がる)。
		_pGE->SubmitUI(
			m_reticleTexRef,	// ResourceRef -> Handle への暗黙変換
			m_screenPos,
			m_size,
			Engine::Color::WHITE
		);
	}

	void CombatReticleHUD::DrawInspector()
	{
		// NDC座標(画面中央が原点、右上が+)
		ImGui::DragFloat2("ScreenPos (NDC)", &m_screenPos.x, 0.01f, -1.0f, 1.0f);
		// NDC半径サイズ
		ImGui::DragFloat2("Size (NDC)", &m_size.x, 0.005f, 0.0f, 2.0f);
	}

	bool CombatReticleHUD::DrawGizmo(const Engine::GameObject::ObjectGizmoContext& a_ctx)
	{
		// UIはスクリーン空間の要素なので3Dギズモは使わず、
		// シーンビュー画像上にドラッグ可能なハンドルを出してNDC座標を編集する。
		if (a_ctx.viewportSize.x <= 0.0f || a_ctx.viewportSize.y <= 0.0f) return false;

		// NDC(-1..1) → ビューポート内ピクセル座標。
		// NDCのYは上が+、スクリーンのYは下方向が+なので反転する。
		ImVec2 _handle;
		_handle.x = a_ctx.viewportPos.x + (m_screenPos.x * 0.5f + 0.5f) * a_ctx.viewportSize.x;
		_handle.y = a_ctx.viewportPos.y + (1.0f - (m_screenPos.y * 0.5f + 0.5f)) * a_ctx.viewportSize.y;

		const float _r = 9.0f;

		// ドラッグ操作用の透明ボタン
		ImGui::SetCursorScreenPos(ImVec2(_handle.x - _r, _handle.y - _r));
		ImGui::InvisibleButton("##CombatReticleGizmo", ImVec2(_r * 2.0f, _r * 2.0f));
		const bool _active = ImGui::IsItemActive();
		const bool _hovered = ImGui::IsItemHovered();

		// ハンドル描画(十字 + 円)
		ImDrawList* _dl = ImGui::GetWindowDrawList();
		const ImU32 _col = _active ? IM_COL32(255, 200, 0, 255)
					   : _hovered ? IM_COL32(255, 255, 255, 255)
							  : IM_COL32(0, 200, 255, 255);
		_dl->AddCircle(_handle, _r, _col, 20, 2.0f);
		_dl->AddLine(ImVec2(_handle.x - _r * 1.8f, _handle.y), ImVec2(_handle.x + _r * 1.8f, _handle.y), _col, 1.5f);
		_dl->AddLine(ImVec2(_handle.x, _handle.y - _r * 1.8f), ImVec2(_handle.x, _handle.y + _r * 1.8f), _col, 1.5f);

		// ドラッグ中はマウス位置からNDC座標を逆算して更新
		if (_active)
		{
			const ImVec2 _mouse = ImGui::GetMousePos();
			const float _u = (_mouse.x - a_ctx.viewportPos.x) / a_ctx.viewportSize.x;	// 0..1
			const float _v = (_mouse.y - a_ctx.viewportPos.y) / a_ctx.viewportSize.y;	// 0..1
			m_screenPos.x = std::clamp(_u * 2.0f - 1.0f, -1.0f, 1.0f);
			m_screenPos.y = std::clamp((1.0f - _v) * 2.0f - 1.0f, -1.0f, 1.0f);
		}

		return true;
	}
}
