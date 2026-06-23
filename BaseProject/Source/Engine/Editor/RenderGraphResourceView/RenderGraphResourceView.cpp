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
		if (!_pGraphicsEngine) { ImGui::End(); return; }

		auto* _pRenderGraph = _pGraphicsEngine->RefRenderGraph();
		if (!_pRenderGraph) { ImGui::End(); return; }

		// レンダーグラフに登録されているテクスチャの取得
		const auto* _pRGResourceManager = _pRenderGraph->GetRGResourceManager();
		if (!_pRGResourceManager) { ImGui::End(); return; }

		// スクロールバー付きの子ウィンドウ領域を作成
		// ImGui::GetContentRegionAvail() を使うことで、メインウィンドウ内の残り領域をすべて埋めます
		if (ImGui::BeginChild("ResourceViewScrollRegion", ImGui::GetContentRegionAvail(), false, ImGuiWindowFlags_AlwaysVerticalScrollbar))
		{
			// リソースを回す
			for (auto& [_name, _index] : _pRGResourceManager->GetNameMap())
			{
				// ノード
				if (ImGui::TreeNodeEx(_name.c_str(), ImGuiTreeNodeFlags_SpanFullWidth | ImGuiTreeNodeFlags_Framed))
				{
					// テクスチャ取得
					auto* _pTex = _pRGResourceManager->GetTex(_index);
					if (!_pTex)
					{
						ImGui::TreePop();
						continue;
					}

					ImGui::Text("name : %s", _name.c_str());

					ImGui::Separator();

					// アスペクト比の計算
					float _aspectRatio = static_cast<float>(a_widht) / static_cast<float>(a_height);

					// 現在のノード内（インデント込み）の利用可能な横幅を取得
					float _availableWidth = ImGui::GetContentRegionAvail().x;

					// 横幅をウィンドウいっぱいにし、アスペクト比から高さを逆算
					float _drawWidth = _availableWidth;
					float _drawHeight = _drawWidth / _aspectRatio;

					// テクスチャ描画
					auto _gpuHandle = D3D12::DescriptorHeapManager::Instance().GetImGuiSRVGPUHandle(_pTex->GetImGuiSRV());
					Helper::DrawSRVView(_gpuHandle, static_cast<UINT>(_drawWidth), static_cast<UINT>(_drawHeight));

					ImGui::TreePop();
				}
			}
		}
		ImGui::EndChild();
	}
	ImGui::End();
}
