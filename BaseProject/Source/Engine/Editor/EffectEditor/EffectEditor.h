#pragma once

namespace Engine
{
	namespace ECS { class World; }
	namespace Resource
	{
		class EffectAsset;
		class ParticlesAsset;
	}
}

namespace Engine::Editor
{
	class EditorCamera;

	//======================================================================================
	//
	// エフェクトエディター
	//
	// エフェクトアセット1枚を、ゲームと同じ描画環境で「見ながら組む」ための画面。
	// アセットインスペクターの「Open Effect Editor」から開く。
	//
	// ・確認用の専用ワールド(エディターシーン)を1つ持つ。
	//   中身は編集中のエフェクトの実体1つだけ。周りに何も無いので、
	//   ゲームのシーンに置いて他のものに埋もれる前に、単体で見え方を詰められる。
	//
	// ・描画は「ゲームのシーンと同じレンダーグラフ」をそのまま通す。
	//   別のパスや簡易プレビューを用意していないのは、そうすると
	//   トーンマップやブルームの掛かり方が本番と変わり、ここで合わせた見た目が
	//   ゲームに持っていくとずれるため。開いている間はゲームのシーンを止めて、
	//   このワールドの描画命令だけをレンダーグラフへ流す。
	//   ただしスカイ(空)だけは消す。薄い粒や加算の抜けが空の色に紛れるため。
	//
	// ・右側で編集する。中身はアセットインスペクターとまったく同じ関数
	//   (EffectAssetEdit / ParticleEdit)を呼んでいるだけで、UIを二重に持っていない。
	//     Effect   タブ : パーツの足し引き・発生位置・発生数・時間指定
	//     Particle タブ : そのパーツが使っている粒そのもの(初速・寿命・絵)
	//   粒まで同じ画面で触れないと、「もう少し細く」のたびに画面を往復することになる。
	//
	// ・カメラはシーンビューと同じ EditorCamera(右クリック中だけ操作するフリーカメラ)。
	//   操作感を変えると、こちらで合わせた画角をシーンで作り直すことになるため。
	//
	// ・開いている間はモーダル。他のパネルもアプリのモード切り替え(O/P)も受け付けない。
	//   ゲームのシーンが止まっている状態でモードを切り替えられると、
	//   「プレイモードなのに何も動かない」という筋の通らない状態になるため。
	//
	//======================================================================================
	class EffectEditor
	{
	public:

		EffectEditor();
		~EffectEditor();

		/// <summary>
		/// 指定のエフェクトを開く(次のフレームからプレビューが回りだす)
		/// </summary>
		void Open(const Engine::GUID& a_effectGUID);

		/// <summary>
		/// 閉じる。ワールドは残し、中に出したエフェクトだけを片付ける
		/// </summary>
		void Close();

		/// <summary>
		/// 開いているか。ここが true の間、ゲームのシーンは止まる
		/// </summary>
		bool IsOpen() const { return m_isOpen; }

		/// <summary>
		/// プレビュー用ワールドの更新(ゲームのシーンの代わりに回る)
		/// </summary>
		void UpdateScene(float a_dt);

		/// <summary>
		/// プレビュー用ワールドの描画命令を積む(ゲームのシーンの代わりに回る)
		/// </summary>
		void DrawScene();

		/// <summary>
		/// カメラの更新。MainEditor がフリーカメラの代わりに呼ぶ
		/// </summary>
		/// <remarks>
		/// シーンビューのフリーカメラと同じく、参照する ImGui の入力は前フレームの
		/// NewFrame 時点のもの。ホバー判定も前フレーム基準なのでずれは生じない
		/// </remarks>
		void UpdateCamera(float a_dt);

		/// <summary>
		/// カメラの割り込み行列を取得する
		/// </summary>
		/// <returns>開いていなければ false(割り込まない)</returns>
		bool TryGetCameraOverride(DXSM::Matrix& a_outWorldMat, DXSM::Matrix& a_outProjMat) const;

		/// <summary>
		/// ポップアップの描画。MainEditor がパネルの後に呼ぶ
		/// </summary>
		void OnDrawImGui();

		/// <summary>
		/// 終了処理
		/// </summary>
		void Release();

	private:

		// プレビュー用ワールドを作る(初回に開いたときだけ)
		void EnsureWorld();

		// 編集中のエフェクトの実体を出す / 片付ける
		void RequestSpawn();
		void DestroyEffectEntity();

		// ワールドの中からプレビュー中のエフェクトを引く。まだ実体化していなければ nullptr
		struct EffectRef;
		EffectRef FindEffect() const;

		// 編集対象のアセット。読み込めていなければ nullptr
		Resource::EffectAsset* RefEffectAsset() const;

		// 今 Particle タブで開いているパーツの粒。選んでいなければ nullptr
		Resource::ParticlesAsset* RefSelectedParticleAsset() const;

		// UI
		void DrawToolbar();
		void DrawViewport();
		void DrawInfo();
		void DrawEditPane();
		void DrawParticleTab();

		// 目安になる格子をデバッグ線で描く(大きさの把握用)
		void DrawGrid() const;

	private:

		//----------------------------------------------------------------------------------
		// 状態
		//----------------------------------------------------------------------------------
		bool m_isOpen = false;			// 開いているか
		bool m_isOpenRequest = false;	// ImGui へポップアップを開かせる要求(開いた最初の1回だけ)

		Engine::GUID m_effectGUID = Engine::DefaultGUID;
		Engine::Handle<Resource::EffectAsset> m_effectHandle = {};

		// プレビュー用のエディターシーン。
		// 閉じても捨てずに使い回す。World::Release() はリソースのGC掃除まで走るので、
		// ゲームのシーンが生きている間に呼ぶとゲーム側のモデルまで解放してしまう
		std::unique_ptr<Engine::ECS::World> m_upWorld = nullptr;

		// シーンビューと同じフリーカメラ。
		// プレビュー専用の実体を持つ(シーンビュー側の位置を動かさないため)
		std::unique_ptr<EditorCamera> m_upCamera = nullptr;

		//----------------------------------------------------------------------------------
		// 再生
		//----------------------------------------------------------------------------------
		bool  m_isPlaying = true;		// 再生中か(止めると時間を進めない)
		bool  m_isLoop = true;			// 出し切ったら頭から繰り返す
		float m_playSpeed = 1.0f;		// 再生速度の倍率
		bool  m_isRestartRequest = false;

		//----------------------------------------------------------------------------------
		// 編集
		//----------------------------------------------------------------------------------
		int m_selectedParticlePart = 0;	// Particle タブで開いているパーツ番号
		float m_editPaneWidth = 420.0f;	// 右の編集欄の幅(px)

		//----------------------------------------------------------------------------------
		// 表示
		//----------------------------------------------------------------------------------
		bool m_isDrawGrid = true;
		float m_gridSize = 10.0f;		// 格子の半径(m)
	};
}
