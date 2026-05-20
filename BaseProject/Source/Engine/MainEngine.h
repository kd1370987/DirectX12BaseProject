#pragma once

namespace Engine
{
	// 前方宣言
	namespace Window
	{
		class NativeWindow;
	}
	namespace Raytracing
	{
		class RayEngine;
	}
	namespace Time
	{
		class TimeManager;
	}
	namespace Resource
	{
		//class AssetDatabase;
	}
	namespace Graphics
	{
		class GraphicsEngine;
		class RenderContext;
	}

	namespace D3D12
	{
		class PipelineStateManager;
	}
	namespace Collision
	{
		class CollisionWorld;
	}

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

	// エンジンクラス
	class MainEngine
	{
	public:

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

		// グラフィックス関係
		void ExcuteDrawCmd();

		// グラフィックスエンジンアクセス
		Graphics::GraphicsEngine* RefGraphicsEngine();

		// レンダーコンテキストアクセス
		const Graphics::RenderContext* GetRenderContext() const;
		Graphics::RenderContext* RefRenderContext();

		// コリジョンワールド
		Collision::CollisionWorld* RefCollisionWorld();

	private:

		// アセットマネージャーの初期化
		void InitializeAssetDatabase();

	private:

		// クラス
		std::unique_ptr<Window::NativeWindow> m_upWindow = nullptr;						// ウィンドウクラス
		std::unique_ptr<Time::TimeManager> m_upTimeManager = nullptr;					// 時間管理クラス
		//std::unique_ptr<Resource::AssetDatabase> m_upAssetDatabase = nullptr;			// アセットのメタ管理
		std::unique_ptr<Graphics::GraphicsEngine> m_upGraphicsEngine = nullptr;			// 描画周りの管理クラス
		std::unique_ptr<D3D12::PipelineStateManager> m_upPipelineStateManager = nullptr;// パイプラインステート管理
		std::unique_ptr<Collision::CollisionWorld> m_upCollisionWorld = nullptr;		// 当たり判定用ワールド

		// エンジン設定
		EngineConfig m_config = {};

	// シングルトン
	private:

		// コンストラクタ・デストラクタ
		MainEngine();
		~MainEngine();

	public:

		static MainEngine& Instance()
		{
			static MainEngine _instance = {};
			return _instance;
		}

	};
}
