#pragma once

class Window;
class TimeManager;

// エンジン設定
struct EngineConfig
{
	// Graphics関連
	struct Graphics
	{
		// 初期設定
		struct Init
		{
			bool isDebugLayer = false;			// GPUデバッグ
			bool isGPUValidation = false;
		} init = {};

		// 実行中
		struct Runtime
		{
			bool isVsync = false;				// 垂直同期
		} runtime = {};
	} graphics;

	// Application関連の初期設定
	struct Application
	{
		// アプリケーションのモード
		enum class Mode
		{
			Game,					// リリースするときと同じモード
			Debug,					// エディターを表示して操作するモード
			Monitoring				// 軽量的で、数値の確認のみのモード
		} mode = Mode::Game;
	} app;
};

class Engine
{
public:

	// コンストラクタ・デストラクタ
	Engine();
	~Engine();

	// 初期化・解放
	void Init(EngineConfig a_config);
	void Release();

	// フレーム関係
	bool BegineFrame();
	void EndFrame();

	// 描画関係
	void BeginDraw();
	void EndDraw();

	// デルタタイム取得
	float GetDeltaTime();

	// モード切替
	void ChangeMode(EngineConfig::Application::Mode a_mode);

private:

	// ウィンドウクラス
	std::unique_ptr<Window> m_upWindow = nullptr;

	// 時間管理クラス
	std::unique_ptr<TimeManager> m_upTimeManager = nullptr;

	
	// エンジン設定
	EngineConfig m_config = {};

	// ウィンドウサイズ
	UINT m_windowWidth = 0;
	UINT m_windowHeight = 0;
};
