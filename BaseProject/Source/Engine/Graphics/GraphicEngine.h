#pragma once


namespace Engine
{
	namespace D3D12
	{
		class GraphicsPSOManager;
		class RootSignatureManager;

		class PipelineStateManager;
	}
}

namespace Engine::Graphics
{
	// 前方宣言
	class RenderGraph;
	class ShapeRenderer;
	class RenderContext;

	// グラフィックスエンジンの初期化に必要な情報
	struct GraphicsEngineDesc
	{
		UINT width = 0;						// ウィンドウの横幅
		UINT height = 0;					// ウィンドウの縦幅

		D3D12::PipelineStateManager* pPipelineStateManager = nullptr;
	};

	// グラフィックスエンジン
	class GraphicsEngine
	{
	public:

		GraphicsEngine();
		~GraphicsEngine();

		// 初期化・解放
		void Init(const GraphicsEngineDesc& a_desc);
		void Release();

		// 描画コマンドの実行
		void ExcuteDrawCmd();

		// フレームの開始・終了処理
		void BegineFrame();
		void EndFrame();

		// アクセサ
		const Graphics::RenderContext* GetRenderContext() const;
		Graphics::RenderContext* RefRenderContext();

	private:
		// レンダーコンテキスト
		// 一フレーム内の描画情報を扱う
		std::vector<std::unique_ptr<RenderContext>> m_upRenderContextVec = {};
		UINT m_currentFrameIndex = 0;

		// PSOやルートシグネチャの管理
		D3D12::PipelineStateManager* m_pPipelineStateManager = nullptr;

		// 形状描画クラス
		std::unique_ptr<ShapeRenderer> m_upShapeRender = nullptr;

		// レンダーグラフ
		std::unique_ptr<RenderGraph> m_upRenderGraph = nullptr;

	};
}