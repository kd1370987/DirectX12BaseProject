#pragma once

namespace Engine
{
	namespace ECS { class World; }
}

namespace Engine::Editor
{
	//======================================================================================
	//
	// エフェクトエディター
	//
	// エフェクトアセット1枚だけを、ゲームと同じ描画環境で確認するための画面。
	// アセットインスペクターの「Open Effect Editor」から開く。
	//
	// ・確認用の専用ワールド(エディターシーン)を1つ持つ。
	//   中身は選んだエフェクトの実体1つだけ。周りに何も無いので、
	//   ゲームのシーンに置いて他のものに埋もれる前に、単体で見え方を詰められる。
	//
	// ・描画は「ゲームのシーンと同じレンダーグラフ」をそのまま通す。
	//   別のパスや簡易プレビューを用意していないのは、そうすると
	//   トーンマップやブルームの掛かり方が本番と変わり、ここで合わせた見た目が
	//   ゲームに持っていくとずれるため。開いている間はゲームのシーンを止めて、
	//   このワールドの描画命令だけをレンダーグラフへ流す。
	//
	// ・カメラは注視点まわりを回る軌道カメラを自前で持つ。
	//   ECSのカメラエンティティは作らず、GraphicsEngine のカメラ割り込み
	//   (フリーカメラと同じ仕組み)へ行列を渡す。
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

		// 選択中のエフェクトの実体を出す / 片付ける
		void RequestSpawn();
		void DestroyEffectEntity();

		// ワールドの中からプレビュー中のエフェクトを引く。まだ実体化していなければ nullptr
		struct EffectRef;
		EffectRef FindEffect() const;

		// 軌道カメラ
		void UpdateCamera();
		void BuildCameraMatrix();

		// UI
		void DrawToolbar();
		void DrawViewport();
		void DrawInfo();

		// 目安になる格子をデバッグ線で描く(大きさの把握用)
		void DrawGrid() const;

	private:

		//----------------------------------------------------------------------------------
		// 状態
		//----------------------------------------------------------------------------------
		bool m_isOpen = false;			// 開いているか
		bool m_isOpenRequest = false;	// ImGui へポップアップを開かせる要求(開いた最初の1回だけ)

		Engine::GUID m_effectGUID = Engine::DefaultGUID;

		// プレビュー用のエディターシーン。
		// 閉じても捨てずに使い回す。World::Release() はリソースのGC掃除まで走るので、
		// ゲームのシーンが生きている間に呼ぶとゲーム側のモデルまで解放してしまう
		std::unique_ptr<Engine::ECS::World> m_upWorld = nullptr;

		//----------------------------------------------------------------------------------
		// 再生
		//----------------------------------------------------------------------------------
		bool  m_isPlaying = true;		// 再生中か(止めると時間を進めない)
		bool  m_isLoop = true;			// 出し切ったら頭から繰り返す
		float m_playSpeed = 1.0f;		// 再生速度の倍率
		bool  m_isRestartRequest = false;

		//----------------------------------------------------------------------------------
		// 軌道カメラ
		//----------------------------------------------------------------------------------
		DXSM::Vector3 m_focusPos = { 0.0f, 0.0f, 0.0f };	// 注視点(エフェクトの発生位置)
		float m_yawDeg = 0.0f;
		float m_pitchDeg = 12.0f;
		float m_distance = 8.0f;
		float m_fovY = 60.0f;

		DXSM::Matrix m_camWorldMat = DXSM::Matrix::Identity;
		DXSM::Matrix m_camProjMat = DXSM::Matrix::Identity;

		// ドラッグ操作は画像の上で押し始めたときだけ始める。
		// 始まったあとは枠の外へ出ても離すまで続けたいので、状態を持つ
		bool m_isOrbiting = false;
		bool m_isPanning = false;

		//----------------------------------------------------------------------------------
		// 表示
		//----------------------------------------------------------------------------------
		bool m_isDrawGrid = true;
		float m_gridSize = 10.0f;		// 格子の半径(m)
	};
}
