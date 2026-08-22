#pragma once

namespace Engine::Resource
{
	class Texture;
}

namespace Engine::Graphics
{
	class GraphicsEngine;

	/// <summary>
	/// 自前で描くマウスカーソル
	/// </summary>
	/// <remarks>
	/// OSのカーソルはウィンドウのクライアント領域では消し、代わりにカーソル位置へ
	/// 設定された画像を描く。設定は CursorOption。
	///
	/// 描く先はアプリのモードで変わる。
	///   ゲーム : UIパスへ積む(GraphicsEngine::Execute の中、UIを全部積み終えた後)
	///   それ以外 : ImGuiの最前面レイヤー(パネルより手前に出さないと隠れて見えなくなる)
	///
	/// OSのカーソルを消すのは「自前の絵を出せているとき」だけにしてある。
	/// 画像が未設定・読み込み中のフレームまで消すとカーソルが1つも無くなり、
	/// ウィンドウを操作できなくなるため。
	/// </remarks>
	class MouseCursor
	{
	public:

		// 初期化・解放
		void Init();
		void Release();

		/// <summary>
		/// 毎フレームの更新 : 設定の反映と、このフレームの位置・可否を決める
		/// </summary>
		/// <remarks>
		/// 描画より前(フレームの頭)に一度だけ呼ぶこと。
		/// ゲーム側とエディター側で2回描くことは無いが、どちらから描かれても
		/// 同じ位置になるよう、位置決めはここに寄せてある。
		/// </remarks>
		void Update();

		// ゲーム画面へ描く : UIを積み終えた最後に呼ぶ(UIパスは積んだ順に重なる)
		void SubmitUI(GraphicsEngine* a_pGraphicsEngine) const;

		// エディター画面へ描く : ImGuiのフレームの中で呼ぶ
		void DrawImGui() const;

		/// <summary>
		/// OSのカーソルを消してよいか
		/// </summary>
		/// <remarks>
		/// ウィンドウ側(WM_SETCURSOR)がこれを見る。
		/// 視点操作でカーソルを画面中央へ固定している間も true のまま
		/// (絵は描かないが、OSのカーソルも出したくないため)。
		/// </remarks>
		bool IsHideOSCursor() const { return m_isHideOSCursor; }

	private:

		// カーソルのクライアント座標を取り、クライアント領域の内側かどうかも見る
		bool TryGetCursorClientPos(Math::Vector2& a_outClientPos) const;

	private:

		// 描画に使うテクスチャ。設定のGUIDが変わったら読み直す
		ResourceRef<Resource::Texture> m_texRef = {};
		Engine::GUID m_loadedGUID = {};

		// OSのカーソルを消してよいか(＝自前の絵を出せる状態か)
		bool m_isHideOSCursor = false;

		// このフレームに絵を描くか
		bool m_isDraw = false;

		// カーソル位置(クライアント座標 px)
		Math::Vector2 m_clientPos = {};
	};
}
