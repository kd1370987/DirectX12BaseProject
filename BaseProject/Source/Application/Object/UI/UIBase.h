#pragma once

#include "../../../Engine/GameObject/BaseObject/BaseObject.h"
#include "Decoration.h"

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

		//-----------------------------------------------------------------------
		// 表示の切り替え
		//-----------------------------------------------------------------------

		/// <summary>
		/// 表示するか
		/// </summary>
		/// <remarks>
		/// 切ると描画されず、押せるUI(UIButton)なら入力も受け取らなくなる。
		/// 画面の中で出したり引っ込めたりするもの(ホームとステージセレクトの
		/// 出し分けなど)は、消さずにこれで切り替える。
		/// </remarks>
		bool IsVisible() const { return m_isVisible; }
		void SetVisible(bool a_isVisible) { m_isVisible = a_isVisible; }

		//-----------------------------------------------------------------------
		// 当たり判定(UIを持たない側からも使えるように静的にしてある)
		//-----------------------------------------------------------------------

		/// <summary>
		/// カーソル位置をUIのピクセル座標(描画解像度基準)で取得する
		/// </summary>
		/// <param name="a_context">入力・オプション・ウィンドウを引くためのコンテキスト</param>
		/// <param name="a_outPos">UIのピクセル座標</param>
		/// <returns>取得できたら true(最小化中などは false)</returns>
		static bool CalcCursorUIPos(Engine::GameObject::ObjectContext& a_context, Math::Vector2& a_outPos);

		/// <summary>
		/// UIのピクセル座標が矩形の内側にあるか
		/// </summary>
		/// <param name="a_uiPos">判定する点(UIのピクセル座標)</param>
		/// <param name="a_pixelPos">ピボットのスクリーン座標(px)</param>
		/// <param name="a_pixelSize">矩形の大きさ(px)</param>
		/// <param name="a_pivot">正規化ピボット[0,1]</param>
		/// <param name="a_rotationDeg">回転(度)</param>
		/// <param name="a_hitPadding">矩形へ足す余白(px)</param>
		static bool IsPointInside(
			const Math::Vector2& a_uiPos,
			const Math::Vector2& a_pixelPos,
			const Math::Vector2& a_pixelSize,
			const Math::Vector2& a_pivot,
			float a_rotationDeg,
			const Math::Vector2& a_hitPadding = {});

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

		// 表示するか。切ると描画も入力も止まる
		bool m_isVisible = true;
	};
}