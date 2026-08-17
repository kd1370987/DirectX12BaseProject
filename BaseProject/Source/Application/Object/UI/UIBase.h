#pragma once

#include "../../../Engine/GameObject/BaseObject/BaseObject.h"

namespace App::Object
{
	class UIBase : public Engine::GameObject::BaseObject
	{
	public:

		// 解放処理
		void Release(Engine::GameObject::ObjectContext& a_context) override;

		// 描画処理
		void Draw(Engine::GameObject::ObjectContext& a_context) override;

		// アーカイブ
		void Archive(Engine::Persistence::Archive& a_ar, Engine::GameObject::ObjectContext& a_context) override;

		//-----------------------------------------------------------------------
		// エディター用
		//-----------------------------------------------------------------------
		
		// UIでの基本的なステータスをいじる : 継承先で作るのなら、初めに呼ぶ
		void DrawInspector(Engine::GameObject::ObjectContext& a_context) override;

		// シーンビュー上のスクリーンハンドルで位置を編集する
		bool DrawGizmo(const Engine::GameObject::ObjectGizmoContext& a_ctx, Engine::GameObject::ObjectContext& a_context) override;

	protected:

		// 描画するUIの構成テクスチャ
		Engine::ResourceRef<Engine::Resource::Texture> m_texRef = {};
		Engine::GUID m_texGUID = {};

		// 色
		Math::Color m_color = Engine::Color::WHITE;

		// 座標系
		Math::Vector2 m_pixelPos = {};
		Math::Vector2 m_pixelSize = {};			// ピクセルサイズ
		float m_rotation = 0.0f;

		// オプション
		Math::Vector2 m_pivot = { 0.5f, 0.5f };	// 回転軸/基準点(正規化[0,1], 0.5=中心)
		Math::Vector2 m_uvOffset = {};			// UVスクロールなど
		float m_layer = 0.0f;					// Z位置
		Math::Vector2 m_editSize = {};			// エディターでいじる際のピクセルサイズ
		float m_scale = 1.0f;					// 等倍スケール用
	};
}