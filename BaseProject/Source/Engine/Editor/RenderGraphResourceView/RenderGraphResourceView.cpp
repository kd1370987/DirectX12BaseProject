#include "RenderGraphResourceView.h"

#include "../../MainEngine.h"
#include "../../Graphics/GraphicEngine.h"
#include "../../Graphics/RenderGraph/RenderGraph.h"
#include "../../Graphics/RenderGraph/RGVarsionManager/RGResourceManager.h"

#include "../../D3D12/DescriptorHeapManager/DescriptorHeapManager.h"
#include "../ImGui/ImGuiHelper/ImGuiHelper.h"

void Engine::Editor::RenderGraphResourceView::Init()
{}

void Engine::Editor::RenderGraphResourceView::Draw(UINT a_widht, UINT a_height)
{
	if (ImGui::Begin("RenderGraphResourceView"))
	{
		// レンダーグラフ取得
		auto* _pGraphicsEngine = MainEngine::Instance().RefGraphicsEngine();
		if (!_pGraphicsEngine) return;
		auto* _pRenderGraph = _pGraphicsEngine->RefRenderGraph();
		if (!_pRenderGraph) return;

		// レンダーグラフに登録されているテクスチャの取得
		const auto* _pRGResourceManager = _pRenderGraph->GetRGResourceManager();
		if (!_pRGResourceManager) return;

		// リソースを回す
		for (auto& [_name, _index] : _pRGResourceManager->GetNameMap())
		{
			// ノード
			if (ImGui::TreeNodeEx(_name.c_str(), ImGuiTreeNodeFlags_SpanFullWidth | ImGuiTreeNodeFlags_Framed))
			{
				// テクスチャ取得
				auto* _pTex = _pRGResourceManager->GetTex(_index);
				if (!_pTex) continue;

				ImGui::Text("name : %s", _name);

				ImGui::Separator();

				// テクスチャ描画
				auto _gpuHandle = D3D12::DescriptorHeapManager::Instance().GetImGuiSRVGPUHandle(_pTex->GetImGuiSRV());
				Helper::DrawSRVView(_gpuHandle, a_widht, a_height);

				ImGui::TreePop();
			}
		}
	}
	ImGui::End();

}
