#pragma once
//==========================================================================================
//
// GraphicsPipeline (Engine::Graphics::Pipeline)
//
// カメラ1台ぶんの描画処理をまとめる上位システム。
//
//   Camera            : 何を見るか
//   GraphicsPipeline  : どの描画構成を使うか        <- ここ
//   RenderGraph       : パス群をどの順で実行するか
//   Pass              : 実際にどのGPU処理を行うか
//
// 内部に RenderGraph を1つ持ち、そのコンパイルと実行を管理する。
// 依存関係やリソースステートの解析そのものには立ち入らない(RenderGraph の責務)。
//
// 実体は設計図(RenderingPipelineAsset)から複製して作る。
// 設計図をそのまま回さないのは、同じアセットを2台のカメラが指したときに
// GBuffer も定数バッファも取り合って壊れるため。
//
//==========================================================================================
#include "../RenderGraph/RenderGraph.h"

namespace Engine::Graphics::Pipeline
{
	class RenderingPipelineAsset;
	class PassMetaRegistry;

	class GraphicsPipeline
	{
	public:

		GraphicsPipeline() = default;
		~GraphicsPipeline() = default;

		// パスの実体を抱えるのでコピー禁止
		GraphicsPipeline(const GraphicsPipeline&) = delete;
		GraphicsPipeline& operator=(const GraphicsPipeline&) = delete;

		//----------------------------------------------------------------------------------
		// 初期化
		//----------------------------------------------------------------------------------
		// 設計図から実行用のグラフを組み立てる。
		// 中のパスは型IDから作り直すので、設計図側とは別の実体になる
		bool BuildFrom(const RenderingPipelineAsset& a_asset, const PassMetaRegistry& a_registry);

		//----------------------------------------------------------------------------------
		// 描画コンテキスト
		//
		// カメラ側の情報のうち、グラフの構築に効くものを流し込む。
		// View / Proj のようにフレームごとに変わるものは Render() の入口で渡す
		//----------------------------------------------------------------------------------
		void SetViewportSize(UINT64 a_width, UINT a_height);

		// グラフの外で作られたリソースを名前で差し込む(このカメラの最終出力など)
		void ImportResource(
			const std::string& a_name,
			D3D12::GPUResource* a_pResource,
			D3D12_RESOURCE_STATES a_initialState = D3D12_RESOURCE_STATE_COMMON,
			EPassSlotType a_type = EPassSlotType::Texture);

		//----------------------------------------------------------------------------------
		// コンパイル / 実行
		//----------------------------------------------------------------------------------
		// 検証 -> 実行順の決定 -> 仮想リソース構築 -> バリア構築 -> 各パスの Compile。
		// a_pDevice を渡すと、続けて物理リソースの割り当てまで済ませる
		bool Compile(GraphicsEngine* a_pGraphicsEngine = nullptr, D3D12::Device* a_pDevice = nullptr);

		// コンパイル済みのグラフを実行する
		void Render(GraphicsEngine* a_pGraphicsEngine, RenderContext* a_pRenderContext);

		// GPUリソースを手放す。
		// DescriptorHeapManager の解放より前に呼ぶこと
		void Release();

		//----------------------------------------------------------------------------------
		// アクセサ
		//----------------------------------------------------------------------------------
		RenderGraph* RefRenderGraph() { return &m_renderGraph; }
		const RenderGraph* GetRenderGraph() const { return &m_renderGraph; }

		// 直近のコンパイルが通っているか
		bool IsCompiled() const { return m_isCompiled; }

	private:

		RenderGraph m_renderGraph = {};

		// Compile() が通っているか。通っていないグラフは実行しない
		bool m_isCompiled = false;
	};
}
