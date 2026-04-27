#pragma once

class RootSignatureManager;

namespace Engine
{
	namespace Resource
	{
		class ShaderManager;
	}

	namespace D3D12
	{
		class GraphicsPSOManager;
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

		// アクセサ
		const Graphics::RenderContext* GetRenderContext() const;
		Graphics::RenderContext* RefRenderContext();

	private:

		// マネージャ構築
		void CreateManager();

		// ルートシグネチャ定義
		void RootSigDefinition();

	private:
		// レンダーコンテキスト
		// 一フレーム内の描画情報を扱う
		std::unique_ptr<RenderContext> m_upRenderContext = nullptr;

		// マネージャー
		std::unique_ptr<Resource::ShaderManager>	m_upShaderManager = nullptr;		// シェーダー管理
		std::unique_ptr<D3D12::GraphicsPSOManager>	m_upGrahicsPSOManager = nullptr;	// PSO管理
		std::unique_ptr<RootSignatureManager>		m_upRootSignatureManager = nullptr;	// ルートシグネチャ管理

		// 形状描画クラス
		std::unique_ptr<ShapeRenderer> m_upShapeRender = nullptr;

		// レンダーグラフ
		std::unique_ptr<RenderGraph> m_upRenderGraph = nullptr;

	};
}