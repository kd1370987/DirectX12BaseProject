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

		/// <summary>
		/// 更新前処理 : カーソルの上に居ると名乗る
		/// </summary>
		/// <remarks>
		/// 重なっているUIのうち手前の1つだけが反応するように、
		/// 判定そのものはここで済ませて、勝ち負けは Update で見る。
		/// 継承先で持つ場合は、先頭で UIBase::PreUpdate を呼ぶこと
		/// </remarks>
		void PreUpdate(Engine::GameObject::ObjectContext& a_context) override;

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
		// カーソルへの反応
		//
		// 判定は UIBase が持つ。押されて何をするかは持たないので、
		// 背景でもパネルでも「乗ったら枠を出す」「押したら縮む」が付けられる。
		// 実際に押されたときの処理を差し込みたいものだけ UIButton を使う
		//
		// 重なったときは **手前の1つだけ** が反応する(Layer が大きいほど手前。
		// 同じ値なら後に描かれるほう)。下になったUIは矩形の中にカーソルが居ても
		// 乗っていない扱いになるので、上のパネルごしに裏のボタンが押せることはない。
		// カーソルを通したい(奥のUIを押させたい)ものは Interactable を切ること
		//-----------------------------------------------------------------------

		// カーソルが乗っているか
		bool IsHovered() const { return m_isHovered; }

		// 押されている最中か(押しっぱなし)
		bool IsPressed() const { return m_isPressed; }

		// このフレームに押し切られたか(押して離した瞬間だけ true)
		bool IsClicked() const { return m_isClicked; }

		// 今の状態 : 飾りの反応(Decoration::UIReaction)へ渡すもの
		Decoration::EUIState GetUIState() const;

		// 触れるかどうか。切ると乗っても押しても反応しない(Disabled 扱い)
		bool IsInteractable() const { return m_isInteractable; }
		void SetInteractable(bool a_isInteractable) { m_isInteractable = a_isInteractable; }

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
		Decoration::ParentOption MakeParentOption() const;

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

		/// <summary>
		/// 飾りを1つだけ描く
		/// </summary>
		/// <remarks>
		/// 飾りごとに違う差し替えを掛けたいとき用(ゲージの中身だけ横に縮める等)。
		/// 自分で順に回して呼べば、配列順の重なりはそのまま保たれる
		/// </remarks>
		void DrawDecorationAt(
			Engine::GameObject::ObjectContext& a_context,
			size_t a_index,
			const Decoration::DrawOverride& a_override = {});

		// 名前から飾りの番号を引く(見つからなければ -1)
		int FindDecorationIndex(const std::string& a_name) const;

		// 保存されているGUIDから、飾りのテクスチャ・フォントを引き直す
		void RequestDecorationResources(Engine::GameObject::ObjectContext& a_context);

		// 飾りの一覧をインスペクターへ出す(追加・削除・並べ替え)
		void DrawDecorationListInspector(Engine::GameObject::ObjectContext& a_context);

		// カーソルの当たり判定と押下の進行(UIBase::Update から)
		void UpdateInteraction(Engine::GameObject::ObjectContext& a_context);

		/// <summary>カーソルを受け取れる状態か(出ていて・触れて・プレイモード中)</summary>
		/// <remarks>
		/// 名乗り(PreUpdate)と進行(UpdateInteraction)で同じ条件を見るために切り出してある。
		/// ここがずれると、反応しないUIが名乗って下のUIを塞ぐ
		/// </remarks>
		bool IsCursorReceivable(Engine::GameObject::ObjectContext& a_context) const;

		// UIのピクセル座標が自分の判定矩形の内側にあるか
		bool IsPointInsideSelf(const Math::Vector2& a_uiPos) const;

		/// <summary>
		/// 飾りが占めている範囲(アンカーからの相対, px)を求める
		/// </summary>
		/// <param name="a_outCenter">範囲の中心(アンカーからのずれ)</param>
		/// <param name="a_outSize">範囲の大きさ</param>
		/// <param name="a_isIncludeAnim">アニメーション・反応で変わった今の大きさも含めるか</param>
		/// <returns>大きさを持つ飾りが1つも無ければ false</returns>
		/// <remarks>
		/// 含める場合も素の矩形は必ず範囲へ入れる(素の矩形と今の矩形の合併を返す)。
		/// 今の矩形だけにすると、乗ると縮む反応を付けたときに
		/// 「乗る→縮んで外れる→戻って乗る」を繰り返してちらつくため
		/// </remarks>
		bool CalcDecorationBounds(
			Math::Vector2& a_outCenter,
			Math::Vector2& a_outSize,
			bool a_isIncludeAnim = false) const;

		/// <summary>
		/// 音を鳴らす
		/// </summary>
		/// <remarks>
		/// インスタンスは初めて鳴らすときに借りる。
		/// UIは画面ぶん並ぶので、鳴らさないものにまで先に確保させると席が尽きる。
		///
		/// 間引きの残り時間が残っているうちは鳴らさない。
		/// インスタンスは1つで Play は頭出しの鳴らし直しになるため、
		/// 判定の縁でカーソルが揺れると毎フレーム鳴り直してしまう
		/// (残響が重なって、だんだん大きくなったように聞こえる)
		/// </remarks>
		void PlayUISound(
			Engine::GameObject::ObjectContext& a_context,
			const Engine::GUID& a_guid,
			Engine::Handle<Engine::Resource::SoundInstance>& a_inoutHandle,
			float& a_inoutCoolTime);

		// 借りている音のインスタンスを返す
		void ReleaseUISounds(Engine::GameObject::ObjectContext& a_context);

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

		// 湾曲
		Math::Vector2 m_curveCenter = {};		// ローカルでの中心点
		float m_curveRadius = 0.0f;				// 半径
		float m_curveAngle = 0.0f;				// 強度


		// 表示するか。切ると描画も入力も止まる
		bool m_isVisible = true;

		//-----------------------------------------------------------------------
		// カーソルへの反応(保存される)
		//-----------------------------------------------------------------------
		// 押下に使う入力アクション名。InputManager へ登録した名前を指す。
		// 名前で持たせているのは、キー割り当てを入力側の登録だけで変えられるようにするため
		std::string m_clickActionName = "UIClick";

		// 当たり判定の余白(px)。見た目より広く/狭く取りたいとき用
		Math::Vector2 m_hitPadding = { 0.0f, 0.0f };

		// 判定を飾りの今の大きさへ追従させるか
		// 立てている間は PixelSize より飾りの範囲が優先される
		bool m_isHitFollowAnim = false;

		// 触れるかどうか。切ると Disabled 扱いになる
		bool m_isInteractable = true;

		// 乗った瞬間 / 押した瞬間に鳴らす音
		Engine::GUID m_hoverSoundGUID = {};
		Engine::GUID m_pressSoundGUID = {};
		float m_soundVolume = 1.0f;

		// 同じ音を鳴らし直す最短間隔(秒)。0 で間引かない
		float m_soundMinInterval = 0.08f;

		//-----------------------------------------------------------------------
		// 状態(保存しない)
		//-----------------------------------------------------------------------
		bool m_isHovered = false;	// カーソルが乗っている(かつ自分が手前)
		bool m_isPressed = false;	// 押されている最中
		bool m_isClicked = false;	// このフレームに押し切られた

		// 矩形の中にカーソルが居るか。PreUpdate で見た結果。
		// これが true でも、手前に別のUIが重なっていれば m_isHovered は false になる
		bool m_isCursorInside = false;

		// 矩形の内側で押し始めたか。
		// これを見ておかないと、外で押してUIの上で離しただけで反応してしまう
		bool m_isPressStartedInside = false;

		// 借りている音のインスタンス。初めて鳴らすときに取り、Release で返す
		Engine::Handle<Engine::Resource::SoundInstance> m_hoverSoundHandle = {};
		Engine::Handle<Engine::Resource::SoundInstance> m_pressSoundHandle = {};

		// 次に鳴らせるまでの残り時間(秒)
		float m_hoverSoundCoolTime = 0.0f;
		float m_pressSoundCoolTime = 0.0f;

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
