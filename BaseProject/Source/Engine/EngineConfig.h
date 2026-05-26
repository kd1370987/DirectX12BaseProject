#pragma once
namespace Engine
{
	// ビルド構成
	enum class EBuildConfiguration
	{
		Debug,			// 最適化なし、デバッグ機能フル稼働
		Development,	// 最適化あり、エディター、プロファイラーなどの開発ツール有効
		Shipping		// リリース用、デバッグ機能、エディタ機能はすべて除外
	};

	// 起動時設定
	struct InitConfig
	{
		// 環境設定
		EBuildConfiguration buildMode = EBuildConfiguration::Development;

		// パス設定
		std::string assetRootPath = "Asset/";

		// D3Dデバッグ
		bool isDebugLayer = false;			// デバイスデバッグ機能
		bool isGPUValidation = false;		// D3Dメモリリーク通知

		// システムリソース
		UINT maxThreadCount = 4;			// 使用できるスレッド最大数
	};

	// アプリケーションモード
	enum class EAppMode
	{
		Game,			// ゲームを遊ぶ時と同じ画面
		Editor,			// エディター画面
		// マルチやモニターモードが追加されればここに
	};

	// ウィンドウモード
	enum class EWindowMode
	{
		Window,		// ウィンドウモード
		FullScreen,	// 排他的フルスクリーン
		Borderless	// ボーダーレスウィンドウ
	};

	// ランタイム時に変更可能な設定
	struct RuntimeConfig
	{
		// ディスプレイ設定
		UINT windowWidth = 1920;
		UINT windowHeight = 1080;
		EWindowMode windowMode = EWindowMode::Window;
		bool isVsync = false;							// 垂直同期

		// パフォーマンス設定
		UINT targetFrameRate = 120;		// 最大フレームレート

		// アプリケーションモード
		EAppMode appMode = EAppMode::Game;

		// メインボリューム
		UINT mainSoundBolume = 50;
	};

	// エンジン設定
	class EngineConfig
	{
	public:

		// 初期化
		void Init(InitConfig a_initConfig, RuntimeConfig a_runtimeConfig)
		{
			if (m_isInit) return;

			m_initConfig = a_initConfig;
			m_runtimeConfig = a_runtimeConfig;
			m_isInit = true;
		}

		// ---- アクセサ ----
		const InitConfig& GetInitConfig()		const { return m_initConfig; }

		const RuntimeConfig& GetRuntimeConfig()	const { return m_runtimeConfig; }
		RuntimeConfig& RefRuntimeConfig()		{ return m_runtimeConfig; }

	private:

		InitConfig m_initConfig = {};
		RuntimeConfig m_runtimeConfig = {};

		bool m_isInit = false;
	};
}