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
	namespace Particle
	{
		class ParticleBufferManager;
	}
	namespace Time
	{
		class TimeManager;
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

	namespace Thread
	{
		class JobSystem;
	}

	// エンジンクラス
	class MainEngine
	{
	public:

		// 初期化・解放
		void Init();
		void Release();

		// フレーム関係
		bool BeginFrame();
		void EndFrame();

		// 描画関係
		void BeginDraw();
		void EndDraw();

		// デルタタイム取得
		UINT GetFPS();
		float GetDeltaTime();

		// モード切替
		void ChangeMode(EAppMode a_mode);
		EAppMode GetMode() { return m_appMode; }

		// グラフィックス関係
		void ExecuteDrawCmd();

		// ウィンドウ取得
		const Window::NativeWindow* GetNativeWindow() const;
		Window::NativeWindow* RefNativeWindow();

		// グラフィックスエンジンアクセス
		Graphics::GraphicsEngine* RefGraphicsEngine();

		// レンダーコンテキストアクセス
		const Graphics::RenderContext* GetRenderContext() const;
		Graphics::RenderContext* RefRenderContext();

		// コリジョンワールド
		Collision::CollisionWorld* RefCollisionWorld();

		// ジョブシステム
		Thread::JobSystem* RefJobSystem();

		// コンフィグ取得
		EBuildConfiguration GetBuildMode() const { return m_buildMode; }

		// パーティクル
		const Particle::ParticleBufferManager* GetParticleManager() const ;
		Particle::ParticleBufferManager* RefParticleManager();

		// パイプラインステートマネージャー
		const D3D12::PipelineStateManager* GetPipelineManager() const;
		D3D12::PipelineStateManager* RefPipelineManager();

		// ============================================================================
		// 遅延開放処理
		// ============================================================================
		// 遅延開放したい処理を登録
		void RegisterDeferredResource(std::function<void()> a_releaseFunc);

	private:

		// アセットマネージャーの初期化
		void InitializeAssetDatabase();

	private:

		// クラス
		std::unique_ptr<Window::NativeWindow> m_upWindow = nullptr;						// ウィンドウクラス
		std::unique_ptr<Time::TimeManager> m_upTimeManager = nullptr;					// 時間管理クラス
		std::unique_ptr<Graphics::GraphicsEngine> m_upGraphicsEngine = nullptr;			// 描画周りの管理クラス
		std::unique_ptr<D3D12::PipelineStateManager> m_upPipelineStateManager = nullptr;// パイプラインステート管理
		std::unique_ptr<Collision::CollisionWorld> m_upCollisionWorld = nullptr;		// 当たり判定用ワールド
		std::unique_ptr<Particle::ParticleBufferManager> m_upParticleManager = nullptr;	// パーティクルマネージャー
		std::unique_ptr<Thread::JobSystem> m_upJobSystem = nullptr;						// ジョブシステム
		// エンジン設定
		EAppMode m_appMode = EAppMode::Editor;								// アプリケーションのモード
		EBuildConfiguration m_buildMode = EBuildConfiguration::Debug;		// ビルドモード

		// フレーム分のごみ箱を用意する
		std::vector<std::function<void()>> m_releaseQueues[CPU_FRAME_COUNT];

	// シングルトン
	private:

		// コンストラクタ・デストラクタ
		MainEngine();
		~MainEngine();
		NON_COPYABLE_NON_MOVABLE(MainEngine);

	public:

		static MainEngine& Instance()
		{
			static MainEngine _instance = {};
			return _instance;
		}

	};
}
