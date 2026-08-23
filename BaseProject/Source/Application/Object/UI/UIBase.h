#pragma once

#include "../../../Engine/GameObject/BaseObject/BaseObject.h"
#include "Decoration.h"

namespace App::Object
{
	//======================================================================================
	// UIの土台
	//
	// UI 1つが持つのは「画面のどこに、どの向き・どの大きさで置くか」だけ。
	// 実際に見えるものは Decoration を配列で生やして作る。
	//
	//   UIBase     … アンカー(位置・大きさ・回転・倍率・色・Z順)
	//   Decoration … そこから相対で出す絵(板ポリ・画像・文字)
	//
	// テクスチャを1枚だけ持たせる作りをやめたのは、枠と文字とアイコンのように
	// 複数の絵で1つのUIを組みたいときに、UIを人数ぶん並べるしかなくなるため。
	// 飾りを配列にしておけば、位置・回転・倍率はアンカーに追従したまま重ねられる。
	//
	// PixelSize はアンカー自身の矩形。当たり判定(UIButton)や
	// 判定円の大きさ(AimReticleHUD / CombatReticleHUD)がこれを見る。
	// 見た目そのものは飾り側の PixelSize が決めるので、両者は別物であることに注意
	//======================================================================================
	class UIBase : public Engine::GameObject::BaseObject
	{
	public:

		// 解放処理
		void Release(Engine::GameObject::ObjectContext& a_context) override;

		// 更新処理 : 飾りのアニメーションを進める
		void Update(Engine::GameObject::ObjectContext& a_context) override;

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
		bool IsVisible() const override { return m_isVisible; }
		void SetVisible(bool a_isVisible) override { m_isVisible = a_isVisible; }

		//-----------------------------------------------------------------------
		// 飾り
		//-----------------------------------------------------------------------

		/// <summary>
		/// 飾りを1つ足して、その参照を返す
		/// </summary>
		/// <remarks>返る参照は次に足すまでの間だけ有効(配列が伸びると動く)</remarks>
		Decoration::Decoration& AddDecoration(Decoration::EDecorationType a_type);

		// 名前で探す : 見つからなければ nullptr
		Decoration::Decoration* FindDecoration(const std::string& a_name);

		// 中身の取得
		std::vector<Decoration::Decoration>& RefDecorations() { return m_decorationVec; }
		const std::vector<Decoration::Decoration>& GetDecorations() const { return m_decorationVec; }

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

		//-----------------------------------------------------------------------
		// 継承先から使う描画
		//-----------------------------------------------------------------------

		// 今のアンカーの状態を Decoration へ渡す形で取り出す
		Decoration::ParentTransform MakeParentTransform() const;

		/// <summary>
		/// 飾りを全部描く
		/// </summary>
		/// <param name="a_override">
		/// その場かぎりの差し替え。
		/// 位置を敵ごとに変える・桁ごとにUVをずらす・状態で色を掛ける、といった
		/// 保存しない上書きはここへ渡す(飾り側の値を書き換えると次のフレームまで残るため)
		/// </param>
		void DrawDecorations(
			Engine::GameObject::ObjectContext& a_context,
			const Decoration::DrawOverride& a_override = {});

		// 保存されているGUIDから、飾りのテクスチャ・フォントを引き直す
		void RequestDecorationResources(Engine::GameObject::ObjectContext& a_context);

		// 飾りの一覧をインスペクターへ出す(追加・削除・並べ替え)
		void DrawDecorationListInspector(Engine::GameObject::ObjectContext& a_context);

	protected:

		//-----------------------------------------------------------------------
		// アンカー(保存される)
		//-----------------------------------------------------------------------

		// 色 : 全ての飾りへ乗算で掛かる
		Math::Color m_color = Engine::Color::WHITE;

		// 座標系
		Math::Vector2 m_pixelPos = {};			// ピボットのスクリーン座標(px)
		Math::Vector2 m_pixelSize = {};			// アンカー自身の大きさ(px) : 当たり判定・判定円の基準
		float m_rotation = 0.0f;

		// オプション
		Math::Vector2 m_pivot = { 0.5f, 0.5f };	// 回転軸/基準点(正規化[0,1], 0.5=中心)
		float m_layer = 0.0f;					// Z位置
		Math::Vector2 m_editSize = {};			// エディターでいじる際のピクセルサイズ
		float m_scale = 1.0f;					// 等倍スケール用

		// 表示するか。切ると描画も入力も止まる
		bool m_isVisible = true;

		// 飾り : 配列の順に描くので、後ろにあるものほど手前に出る
		std::vector<Decoration::Decoration> m_decorationVec = {};

	private:

		//-----------------------------------------------------------------------
		// 旧形式(テクスチャ1枚)からの引き継ぎ用
		//
		// 既存のシーンは UIBase が直接テクスチャを持っていた頃の値で保存されている。
		// 並びを変えずにここへ読み込んでおき、飾りの配列を持たないシーンだけ
		// 画像の飾り1つへ移し替える
		//-----------------------------------------------------------------------
		Engine::GUID m_legacyTexGUID = {};
		Math::Vector2 m_legacyUvOffset = {};

		// エディターで開いている飾りの番号(-1 で未選択)。保存しない
		int m_editDecorationIndex = -1;
	};
}
