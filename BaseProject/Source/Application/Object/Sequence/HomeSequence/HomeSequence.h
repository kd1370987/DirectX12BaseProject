#pragma once

#include "../../../../Engine/GameObject/BaseObject/BaseObject.h"

namespace App::Object
{
	class UIBase;

	/// <summary>
	/// ホーム画面で今どこを見ているか
	/// </summary>
	enum class EHomeMode : int
	{
		Home = 0,		// トップ : ステージセレクト / 倉庫 のボタンだけ
		StageSelect,	// ステージセレクト : 左に一覧、右に詳細
	};

	/// <summary>
	/// ホーム画面の進行役 : シーンに一つ置く
	/// </summary>
	/// <remarks>
	/// タイトルから来る画面。持っているのは「今どこを見ているか」と
	/// 「どのステージへ出撃するか」だけで、絵の出し方は UI 側に任せる。
	///
	///   トップ         : ステージセレクト / 倉庫 のボタンを出す
	///   ステージセレクト : 左に一覧を縦へ並べ、カーソルが乗った(または選んだ)ステージの
	///                    画像・説明文・出撃ボタンを右へ出す
	///
	/// ・ボタン(UIButton)はシーンへ置いて GUID で引く
	///     TitleSequence と同じ作り。押されて何をするかはここが差し込むので、
	///     ボタン側はホームの知識を持たない。
	///
	/// ・ステージ一覧はオブジェクトを置かずに、ここが直接並べて描く
	///     ステージは増減するものなので、1件ずつシーンへ置くと数を変えるたびに
	///     配置し直しになる。並べ方(開始位置・大きさ・間隔)だけを持たせて、
	///     項目そのものは設定した数だけその場で描く。
	///     押下判定は UIBase の共通実装を使うので、UIButton と同じ当たり方になる。
	///
	/// ・出し分けは UIBase の Visible
	///     トップとステージセレクトで消したり出したりするものは、
	///     オブジェクトを作り直さずに表示だけ切り替える。
	///     決まった4つのボタンのほかに、背景やロゴを足したいとき用に
	///     「トップで出すUI」「セレクトで出すUI」も持たせてある。
	///
	/// ・倉庫はまだ無いので、ボタンだけ置いて押しても何もしない
	///     既定では押せない状態(灰色)にしてある。作れたら遷移先を足す。
	///
	/// ※ 文字を出す仕組みがエンジンに無いので、ステージ名も説明文も画像で用意する。
	/// </remarks>
	class HomeSequence : public Engine::GameObject::BaseObject
	{
	public:

		// 更新処理 : ボタンへの差し込みと、一覧の選択
		void Update(Engine::GameObject::ObjectContext& a_context) override;

		// 描画処理 : ステージ一覧と、選んだステージの画像・説明文
		void Draw(Engine::GameObject::ObjectContext& a_context) override;

		// 解放処理 : カーソル固定を設定値へ戻す
		void Release(Engine::GameObject::ObjectContext& a_context) override;

		// アーカイブ
		void Archive(Engine::Persistence::Archive& a_ar, Engine::GameObject::ObjectContext& a_context) override;

		//=======================================================================
		// エディター用
		//=======================================================================

		// ヒエラルキー/インスペクター表示名
		const char* GetEditorName() const override { return "HomeSequence"; }

		// インスペクター : ボタン・ステージ一覧・並べ方の設定
		void DrawInspector(Engine::GameObject::ObjectContext& a_context) override;

	private:

		/// <summary>
		/// 一覧に並べる1ステージぶんの設定
		/// </summary>
		struct StageEntry
		{
			// エディターで見分けるための名前(画面には出ない)
			std::string name = "Stage";

			//---- 出す絵(文字を出す仕組みが無いので、名前も説明文も画像) ----
			Engine::GUID listTexGUID  = {};	// 左の一覧に並べる絵(ステージ名)
			Engine::GUID imageTexGUID = {};	// 右に出すステージ画像
			Engine::GUID descTexGUID  = {};	// 右に出す説明文

			// 出撃先
			Engine::GUID sceneGUID = {};

			//---- ランタイム(保存しない) ----
			Engine::ResourceRef<Engine::Resource::Texture> listTexRef  = {};
			Engine::ResourceRef<Engine::Resource::Texture> imageTexRef = {};
			Engine::ResourceRef<Engine::Resource::Texture> descTexRef  = {};
		};

		//-------------------------------------------------------------------
		// 進行
		//-------------------------------------------------------------------

		// ボタンへ押下時の処理を差し込む(まだ見つからなければ何もしない)
		void TryBindButtons(Engine::GameObject::ObjectContext& a_context);

		// 見ている場所の切り替えを頼む(実際に切り替わるのは次の Update)
		// ボタンのコールバックからはコンテキストを持てないので、ここで受けて Update で処理する
		void RequestMode(EHomeMode a_mode);

		// 見ている場所を切り替える(表示の出し分けもここで行う)
		void SetMode(EHomeMode a_mode, Engine::GameObject::ObjectContext& a_context);

		// 今のモードに合わせてUIの表示を切り替える
		void ApplyVisible(Engine::GameObject::ObjectContext& a_context);

		// ステージ一覧のカーソル判定と選択
		void UpdateStageList(Engine::GameObject::ObjectContext& a_context);

		// 選んでいるステージへ出撃する
		void RequestSortie();

		//-------------------------------------------------------------------
		// 小物
		//-------------------------------------------------------------------

		// GUID から UI を引く(UI 以外・見つからない場合は nullptr)
		UIBase* FindUI(Engine::GameObject::ObjectContext& a_context, const Engine::GUID& a_guid) const;

		// 一覧の絵を読み込み直す(GUIDを差し替えたときにも呼ぶ)
		void RequestStageTextures(Engine::GameObject::ObjectContext& a_context);

		// 一覧の a_index 番目を置く位置(ピボットのスクリーン座標)
		Math::Vector2 CalcItemPos(int a_index) const;

		// 右へ出すステージ(カーソルが乗っていればそれ、無ければ選んでいるもの)
		const StageEntry* GetShowEntry() const;

	private:

		//-------------------------------------------------------------------
		// 設定(保存される) : ボタン
		//-------------------------------------------------------------------
		Engine::GUID m_stageSelectButtonGUID = {};	// トップ : ステージセレクトへ
		Engine::GUID m_warehouseButtonGUID   = {};	// トップ : 倉庫(まだ何もしない)
		Engine::GUID m_sortieButtonGUID      = {};	// セレクト : 出撃
		Engine::GUID m_backButtonGUID        = {};	// セレクト : トップへ戻る(任意)

		// 上の4つ以外に出し分けたいUI(背景・見出しなど)
		std::vector<Engine::GUID> m_homeUIGUIDVec   = {};	// トップでだけ出す
		std::vector<Engine::GUID> m_selectUIGUIDVec = {};	// ステージセレクトでだけ出す

		//-------------------------------------------------------------------
		// 設定(保存される) : ステージ
		//-------------------------------------------------------------------
		std::vector<StageEntry> m_stageVec = {};

		//-------------------------------------------------------------------
		// 設定(保存される) : 並べ方
		//-------------------------------------------------------------------
		// 一覧(左) : 1番上の項目の位置から下へ送っていく
		Math::Vector2 m_listOrigin   = { 200.0f, 320.0f };	// 1番上の項目の位置(px)
		Math::Vector2 m_listItemSize = { 420.0f,  72.0f };	// 項目1つの大きさ(px)
		Math::Vector2 m_listPivot    = {   0.0f,   0.5f };	// 既定は左揃え
		float         m_listSpacing  = 18.0f;				// 項目と項目の間隔(px)

		// 一覧の色 : 通常 / カーソルが乗っている / 選んでいる
		Math::Color m_listNormalColor = { 0.75f, 0.75f, 0.75f, 1.0f };
		Math::Color m_listHoverColor  = { 1.0f,  1.0f,  1.0f,  1.0f };
		Math::Color m_listSelectColor = { 1.0f,  0.85f, 0.35f, 1.0f };

		// 詳細(右) : ステージ画像と説明文
		Math::Vector2 m_imagePos   = { 1260.0f, 420.0f };
		Math::Vector2 m_imageSize  = {  880.0f, 495.0f };
		Math::Vector2 m_imagePivot = {    0.5f,   0.5f };

		Math::Vector2 m_descPos   = { 1260.0f, 800.0f };
		Math::Vector2 m_descSize  = {  880.0f, 240.0f };
		Math::Vector2 m_descPivot = {    0.5f,   0.5f };

		// ここが出す絵のZ順(シーンへ置いた背景より手前に来るようにする)
		float m_layer = 0.0f;

		// 一覧を選ぶのに使う入力アクション名。UIButton と揃えておく
		std::string m_clickActionName = "UIClick";

		//-------------------------------------------------------------------
		// 設定(保存される) : ふるまい
		//-------------------------------------------------------------------
		// 倉庫のボタンを押せる状態にするか。中身がまだ無いので既定は切ってある
		bool m_isWarehouseInteractable = false;

		// ホームの間はカーソルの中央固定を切るか(固定したままだと狙えない)
		bool m_isReleaseCursorLock = true;

		//-------------------------------------------------------------------
		// 状態(保存しない)
		//-------------------------------------------------------------------
		EHomeMode m_mode = EHomeMode::Home;	// 今見ている場所

		// ボタンから頼まれた切り替え(Update で処理して落とす)
		EHomeMode m_requestedMode = EHomeMode::Home;
		bool m_isModeRequested = false;

		int m_selectIndex = 0;	// 選んでいるステージ(出撃するのはこれ)
		int m_hoverIndex  = -1;	// カーソルが乗っているステージ(-1 で無し)

		// 押し始めた項目。UIButton と同じで、押し始めと離しが同じ項目のときだけ選ばせる
		// (押したまま外へ逃がせば取り消せる)
		int m_pressIndex = -1;

		bool m_isBound          = false;	// ボタンへ差し込み済みか
		bool m_isTexRequested   = false;	// 一覧の絵を要求済みか
		bool m_isSceneRequested = false;	// 二重に遷移要求を出さないための印
	};
}
