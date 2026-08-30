#include "GraphicsPipeline.h"

#include "../RenderingPipeline.h"
#include "../RenderingPipelineMetaRegistry.h"

namespace Engine::Graphics::Pipeline
{
	bool GraphicsPipeline::BuildFrom(const RenderingPipelineAsset& a_asset, const PassMetaRegistry& a_registry)
	{
		m_isCompiled = false;

		const RenderGraph* _pSource = a_asset.GetRenderGraph();
		if (!_pSource)
		{
			ENGINE_WARNING("[GraphicsPipeline] 設計図のグラフが空です : %s", a_asset.GetName().c_str());
			return false;
		}

		return m_renderGraph.BuildFrom(*_pSource, a_registry);
	}

	void GraphicsPipeline::SetViewportSize(UINT64 a_width, UINT a_height)
	{
		m_renderGraph.SetViewportSize(a_width, a_height);
	}

	void GraphicsPipeline::ImportResource(
		const std::string& a_name,
		D3D12::GPUResource* a_pResource,
		D3D12_RESOURCE_STATES a_initialState,
		EPassSlotType a_type)
	{
		m_renderGraph.ImportResource(a_name, a_pResource, a_initialState, a_type);
	}

	// 解析そのものは RenderGraph 側。ここは順番を守って呼ぶだけにする
	bool GraphicsPipeline::Compile(GraphicsEngine* a_pGraphicsEngine, D3D12::Device* a_pDevice)
	{
		m_isCompiled = false;

		// 検証・実行順・仮想リソース・バリアまで(GPUには触らない)
		if (!m_renderGraph.Compile()) return false;

		// デバイスをもらえていれば実体の割り当てまで済ませる。
		// エディターから構成だけ確かめたいときは渡さずに呼べる
		if (a_pDevice)
		{
			if (!m_renderGraph.AllocateResources(a_pGraphicsEngine, a_pDevice)) return false;
		}

		m_isCompiled = true;
		return true;
	}

	void GraphicsPipeline::Render(GraphicsEngine* a_pGraphicsEngine, RenderContext* a_pRenderContext)
	{
		// コンパイルが通っていないグラフは、リソースの実体も
		// バリアの前提も揃っていないので走らせない
		if (!m_isCompiled) return;

		m_renderGraph.Execute(a_pGraphicsEngine, a_pRenderContext);
	}

	void GraphicsPipeline::Release()
	{
		m_renderGraph.ReleaseResources();
		m_isCompiled = false;
	}
}
