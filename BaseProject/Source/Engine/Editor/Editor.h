#pragma once
namespace Engine::Editor
{
	// 前方宣言
	class LogPanel;
	class ComponentEdit;
	class ImGuiContext;
	class EditorCamera;
	class PanelManager;
	class Profiler;

	//=======================================================================
	// 
	// メインエディタクラス
	// 
	//=======================================================================
	class MainEditor
	{
	public:

		// 初期化
		bool Init(HWND a_hwnd);

		// 解放
		void Release();

		// 更新
		void Update(float a_dt);

		// 描画
		void Draw(D3D12::GraphicsCommandList* a_pCmdList);

		/// <summary>
		/// エディター側に残っている入力を捨てる
		/// </summary>
		/// <remarks>
		/// アプリのモードを切り替えるときに呼ぶ。押しっぱなしのキー・溜まった入力イベント・
		/// フリーカメラの操作中フラグを、切り替えの向こう側へ持ち越さないようにする。
		/// </remarks>
		void ResetInput();

		/// <summary>
		/// シーンが切り替わることをエディターへ知らせる
		/// </summary>
		/// <remarks>
		/// シーンを捨てる・積み替える直前に呼ぶ。エディターが覚えている選択
		/// (エンティティID・ゲームオブジェクトのポインタ)は切り替え先のシーンには
		/// 存在しないので、ここで捨てる。持ち越すと解放済みのオブジェクトを
		/// インスペクターが描きにいって落ちる。
		/// </remarks>
		void OnSceneChanged();

		//=======================================================================
		// ログ関連
		//=======================================================================
	public:
		// ログの追加
		// 文字列と可変引数でログを追加する関数
		void AddLog(const char* a_fmt, ...);
		void AddLogVector(const float* a_data, const size_t& a_size);
		// 行列をログに追加する関数
		void AddLogMatrix(const std::string& a_name, const DirectX::XMFLOAT4X4& a_mat);

		// 処理はおとさないが、不都合な処理が走った警告を出す用のログ
		void WarningLog(const char* a_fmt, ...);

		// 処理を落とす際に呼び出す
		void ErrorLog(const char* a_fmt, ...);

		//=======================================================================
		// 計測関連
		// 実測はProfilerが行う。ここはその入口だけを提供する
		//=======================================================================
		// フレームの開始・終了 : メインループの先頭と末尾で呼ぶ
		void BeginProfileFrame();
		void EndProfileFrame();

		// GPU計測結果の読み戻し : GPU待機を抜けた直後に呼ぶ
		void CollectGPUProfileResult();

		// 計測開始 : コマンドリストを渡すとGPU時間も測る
		void StartTimer(const std::string& a_name, D3D12::GraphicsCommandList* a_pCmdList = nullptr);
		// 計測終了
		void StopTimer(const std::string& a_name, D3D12::GraphicsCommandList* a_pCmdList = nullptr);

		// プロファイラの取得
		Profiler* RefProfiler() { return m_upProfiler.get(); }

		//=======================================================================
		// デバッグ描画用
		//=======================================================================
		// ライン描画
		void DrawLine(
			const DirectX::SimpleMath::Vector3& a_startPos,
			const DirectX::SimpleMath::Vector3& a_endPos,
			const DirectX::SimpleMath::Color& a_color = Color::WHITE
		);

		void DrawBox(const DirectX::SimpleMath::Matrix& a_worldMat, const DirectX::SimpleMath::Color& a_color = Color::WHITE);
		void DrawBox(const DirectX::BoundingBox& a_aabb, const DirectX::SimpleMath::Color& a_color = Color::WHITE);
		void DrawBox(const DirectX::BoundingOrientedBox& a_obb, const DirectX::SimpleMath::Color& a_color = Color::WHITE);
		void DrawCapsule(const DirectX::SimpleMath::Matrix& a_worldMat, const DirectX::SimpleMath::Color& a_color = Color::WHITE);
		void DrawSphere(const DirectX::SimpleMath::Matrix& a_worldMat, const DirectX::SimpleMath::Color& a_color = Color::WHITE);
		void DrawSphere(const DirectX::BoundingSphere& a_sphere, const DirectX::SimpleMath::Color& a_color = Color::WHITE);

		// レイとヒット時に球体を出す
		void DrawRay(
			const DirectX::SimpleMath::Vector3& a_startPos,
			const DirectX::SimpleMath::Vector3& a_dir,
			float a_length,
			bool a_isHit,
			const DirectX::SimpleMath::Color& a_color = Color::WHITE
		);

		void ClearBuffer();
		const std::vector<Graphics::DebugLineData>& GetDebugLineDataVec() const;



		//=======================================================================
		// デバッグ描画関数登録
		//=======================================================================
		void RegisterEditFunc(std::function<void()> a_func);

		//=======================================================================
		// エディター用フリーカメラ
		//=======================================================================
		EditorCamera* RefEditorCamera() { return m_upEditorCamera.get(); }

	private:

		// デバッグ形状を1つ積んでよいか。
		// オプション(DebugDrawOption::drawWire)のオンオフと、バッファの空きをまとめて見る。
		// 各 Draw～ の入口はここだけを見ればよい
		bool CanPushDebugShape();

	private:

		// ImGuiコンテキスト
		std::unique_ptr<ImGuiContext> m_upImGuiContext = nullptr;

		// パネルマネージャー
		std::unique_ptr<PanelManager> m_upPanelManager = nullptr;

		// ログパネル : 実体は PanelManager が所有する。
		// パネルは登録後に増減しないので、ここでは参照だけを持っておく
		LogPanel* m_pLogPanel = nullptr;

		// 計測機 : 計測と集計はここが持ち、パネルは結果を読むだけ
		std::unique_ptr<Profiler> m_upProfiler = nullptr;

		// エディター用フリーカメラ
		std::unique_ptr<EditorCamera> m_upEditorCamera = nullptr;

		// エディター用関数登録
		std::vector<std::function<void()>> m_editFuncVec = {};

		// デバッグ描画用データ
		std::vector<Graphics::DebugLineData> m_debugLineDataVec = {};
		UINT m_debugLineDataCapacity = 10000;

		bool m_isInit = false;

	private:
		MainEditor();
		~MainEditor();
	public:
		// コピー禁止
		MainEditor(const MainEditor&) = delete;
		MainEditor& operator=(const MainEditor&) = delete;
		// ムーブ禁止
		MainEditor(MainEditor&&) = delete;
		MainEditor& operator=(MainEditor&&) = delete;

		// シングルトンインスタンス取得
		static MainEditor& Instance()
		{
			static MainEditor _instance;
			return _instance;
		}
	};
}