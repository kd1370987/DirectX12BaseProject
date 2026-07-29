#pragma once
namespace Engine
{


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


	// ランタイム時に変更可能な設定
	struct RuntimeConfig
	{

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