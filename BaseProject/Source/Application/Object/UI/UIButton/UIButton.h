#pragma once

#include "../UIBase.h"

namespace App::Object
{
	/// <summary>
	/// ボタンの見た目の状態
	/// </summary>
	enum class EUIButtonState : int
	{
		Normal = 0,		// 何もされていない
		Hovered,		// カーソルが乗っている
		Pressed,		// 押されている最中
		Disabled,		// 押せない(m_isInteractable == false)
	};

	/// <summary>
	/// 押せるUI : タイトル・ホーム・リザルトなど、ボタンが要る場所ならどこでも使える
	/// </summary>
	/// <remarks>
	/// 役割は「押されたことを知らせる」ところまでで、押されて何をするかは持たない。
	/// 実際の処理は SetOnClick で外から差し込む。こうしておけば、シーンを増やすたびに
	/// ボタンを継承して作り直す必要が無くなる。
	///
	///     _pButton->SetOnClick([]() { シーンを切り替える等 });
	///
	/// 押した/乗っている といった状態は IsHovered / IsPressed / IsClicked でも取れるので、
	/// コールバックを使わずに呼び出し側で毎フレーム見に行く形でもよい。
	///
	/// ・当たり判定は UIBase のアンカー矩形(PixelPos / PixelSize / Pivot / Rotation)を使う。
	///   飾りは何枚でも生やせるので、そのどれかではなくアンカーを唯一の基準にしてある。
	///   絵に合わせたいときは、アンカーの大きさを絵に合わせること。
	/// ・押下は「押し始めも離しも矩形の内側」で成立させる。押したまま外へ逃がせば
	///   取り消せる、よくあるボタンの作法に合わせてある。
	/// ・入力はプレイモード中しか受け取らない(InputManager 側で止まる)。
	///   エディター操作でボタンが光ったり押されたりしない。
	///
	/// ※ カーソルを画面中央へ固定する設定(InputOption)が入っていると、カーソルは毎フレーム
	///    中央へ戻されるため中央のボタンしか押せない。メニューを出す場面では固定を切ること。
	/// </remarks>
	class UIButton : public UIBase
	{
	public:

		//=======================================================================
		// ふるまいの差し替え
		//=======================================================================

		/// <summary>
		/// 押されたときに呼ぶ処理を設定する
		/// </summary>
		/// <param name="a_callback">押し切られた瞬間に1回だけ呼ばれる</param>
		void SetOnClick(std::function<void()> a_callback)
		{
			m_onClick = std::move(a_callback);
		}

		/// <summary>
		/// 設定した処理を外す
		/// </summary>
		void ClearOnClick() { m_onClick = nullptr; }

		//=======================================================================
		// 状態の取得
		//=======================================================================

		// カーソルが乗っているか
		bool IsHovered() const { return m_isHovered; }

		// 押されている最中か(押しっぱなし)
		bool IsPressed() const { return m_isPressed; }

		// このフレームに押し切られたか(押して離した瞬間だけ true)
		bool IsClicked() const { return m_isClicked; }

		// 今の見た目の状態
		EUIButtonState GetState() const;

		// 押せるかどうか
		bool IsInteractable() const { return m_isInteractable; }
		void SetInteractable(bool a_isInteractable) { m_isInteractable = a_isInteractable; }

		//=======================================================================
		// オブジェクト
		//=======================================================================

		// 更新処理 : 当たり判定と押下の進行
		void Update(Engine::GameObject::ObjectContext& a_context) override;

		// 描画処理 : 状態に応じた色で描く
		void Draw(Engine::GameObject::ObjectContext& a_context) override;

		// アーカイブ
		void Archive(Engine::Persistence::Archive& a_ar, Engine::GameObject::ObjectContext& a_context) override;

		//=======================================================================
		// エディター用
		//=======================================================================

		// ヒエラルキー/インスペクター表示名
		const char* GetEditorName() const override { return "UIButton"; }

		// インスペクター
		void DrawInspector(Engine::GameObject::ObjectContext& a_context) override;

	private:

		// UIのピクセル座標が自分の矩形の内側にあるか
		// (判定そのものは UIBase の共通実装。自分の位置・大きさを渡すだけ)
		bool IsPointInsideSelf(const Math::Vector2& a_uiPos) const;

		// 今の状態に対応する色を返す(飾りの色へ乗算で掛ける)
		Math::Color GetStateColor() const;

	private:

		// 押されたときに呼ぶ処理(設定されていなければ何もしない)
		std::function<void()> m_onClick = nullptr;

		//-------------------------------------------------------------------
		// 設定(保存される)
		//-------------------------------------------------------------------
		// 押下に使う入力アクション名。InputManager へ登録した名前を指す。
		// 名前で持たせているのは、キー割り当てを入力側の登録だけで変えられるようにするため。
		std::string m_clickActionName = "UIClick";

		// 状態ごとの色。UIBase の Color へ乗算で掛かる(白のままなら見た目が変わらない)
		Math::Color m_hoverColor    = { 1.0f, 1.0f, 1.0f, 1.0f };
		Math::Color m_pressColor    = { 0.7f, 0.7f, 0.7f, 1.0f };
		Math::Color m_disableColor  = { 0.4f, 0.4f, 0.4f, 0.6f };

		// 当たり判定の余白(px)。見た目より広く/狭く取りたいとき用
		Math::Vector2 m_hitPadding = { 0.0f, 0.0f };

		// 押せるかどうか
		bool m_isInteractable = true;

		//-------------------------------------------------------------------
		// 状態(保存しない)
		//-------------------------------------------------------------------
		bool m_isHovered = false;	// カーソルが乗っている
		bool m_isPressed = false;	// 押されている最中
		bool m_isClicked = false;	// このフレームに押し切られた

		// 矩形の内側で押し始めたか。
		// これを見ておかないと、外で押してボタンの上で離しただけで反応してしまう
		bool m_isPressStartedInside = false;
	};
}
