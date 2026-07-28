#include "CombatReticleHUD.h"

#include "Engine/MainEngine.h"
#include "Engine/Graphics/GraphicEngine.h"
#include "Engine/Resource/Manager/ResourceManager/ResourceManager.h"
#include "Engine/Resource/Manager/AssetDatabase/AssetDatabase.h"
#include "Engine/Common/Color.h"
#include "Engine/Option/OptionManager.h"	// ウィンドウ解像度(px)取得用

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

		// UIシェーダ(UIVS.hlsl)はpos/sizeをNDC(クリップ空間)で扱う。
		// 編集はピクセルで行いたいので、現在のウィンドウ解像度を使ってピクセル→NDCへ変換する。
		const auto& _winOp = Engine::Option::OptionManager::GetInstance().GetWindowOption();
		const float _w = static_cast<float>(_winOp.windowWidth);
		const float _h = static_cast<float>(_winOp.windowHeight);
		if (_w <= 0.0f || _h <= 0.0f) return;

		// 中心座標 : ピクセル(左上原点/Y下向き) → NDC(中心原点/Y上向き)
		DXSM::Vector2 _ndcPos;
		_ndcPos.x = m_posPixel.x / _w * 2.0f - 1.0f;
		_ndcPos.y = 1.0f - m_posPixel.y / _h * 2.0f;

		// サイズ : フルサイズ(px) → NDC半径。
		// ベースクアッドが-1〜1(全幅2)で size 分だけ拡縮するため、半径 = px / 解像度 となる。
		DXSM::Vector2 _ndcSize;
		_ndcSize.x = m_sizePixel.x / _w;
		_ndcSize.y = m_sizePixel.y / _h;

		_pGE->SubmitUI(
			m_reticleTexRef,	// ResourceRef -> Handle への暗黙変換
			_ndcPos,
			_ndcSize,
			Engine::Color::WHITE
		);
	}

	void CombatReticleHUD::DrawInspector()
	{
		// スクリーン座標(ピクセル : 左上原点、矩形の中心を指す)。1px単位で移動。
		ImGui::DragFloat2("Position (px)", &m_posPixel.x, 1.0f);
		// 表示サイズ(ピクセル : 幅・高さ)。1px単位で拡縮。
		ImGui::DragFloat2("Size (px)", &m_sizePixel.x, 1.0f, 0.0f, 8192.0f);
	}

	bool CombatReticleHUD::DrawGizmo(const Engine::GameObject::ObjectGizmoContext& a_ctx)
	{
		// UIはスクリーン空間の要素なので3Dギズモは使わず、
		// シーンビュー画像上にドラッグ可能なハンドルを出してピクセル座標を編集する。
		if (a_ctx.viewportSize.x <= 0.0f || a_ctx.viewportSize.y <= 0.0f) return false;

		// 変換に使うゲーム解像度(px)を取得。
		const auto& _winOp = Engine::Option::OptionManager::GetInstance().GetWindowOption();
		const float _w = static_cast<float>(_winOp.windowWidth);
		const float _h = static_cast<float>(_winOp.windowHeight);
		if (_w <= 0.0f || _h <= 0.0f) return false;

		// ゲーム内ピクセル(左上原点) → シーンビュー画像上のピクセル。
		// どちらも左上原点・Y下向きなので反転は不要。画像は解像度と表示サイズの比でスケールする。
		ImVec2 _handle;
		_handle.x = a_ctx.viewportPos.x + (m_posPixel.x / _w) * a_ctx.viewportSize.x;
		_handle.y = a_ctx.viewportPos.y + (m_posPixel.y / _h) * a_ctx.viewportSize.y;

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

		// ドラッグ中はマウス位置からピクセル座標を逆算して更新
		if (_active)
		{
			const ImVec2 _mouse = ImGui::GetMousePos();
			const float _u = (_mouse.x - a_ctx.viewportPos.x) / a_ctx.viewportSize.x;	// 0..1
			const float _v = (_mouse.y - a_ctx.viewportPos.y) / a_ctx.viewportSize.y;	// 0..1
			m_posPixel.x = std::clamp(_u * _w, 0.0f, _w);
			m_posPixel.y = std::clamp(_v * _h, 0.0f, _h);
		}

		return true;
	}
}
