#include "RenderGraphResourceViewPanel.h"

#include "../../../MainEngine.h"

// グラフィックス系
#include "../../../Graphics/GraphicEngine.h"
#include "../../../Graphics/RenderGraph/RenderGraph.h"
#include "../../../Graphics/RenderGraph/RGVarsionManager/RGResourceManager.h"

// D3D系
#include "../../../D3D12/DescriptorHeapManager/DescriptorHeapManager.h"

namespace Engine::Editor
{
	void RenderGraphResourceViewPanel::OnDrawImGui(EditorContext& a_editContext)
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
		if (ImGui::BeginChild("ResourceViewScrollRegion", ImGui::GetContentRegionAvail(), false, ImGuiWindowFlags_AlwaysVerticalScrollbar))
		{

			// リソースを回す
			for (auto& _upTempTex : _pRGResourceManager->GetTempTextures())
			{
				if (!_upTempTex) continue;
				// ノード
				if (ImGui::TreeNodeEx(_upTempTex->GetName().c_str(), ImGuiTreeNodeFlags_SpanFullWidth | ImGuiTreeNodeFlags_Framed))
				{
					ImGui::Text("name : %s", _upTempTex->GetName().c_str());

					ImGui::Separator();

					// アスペクト比の計算
					float _aspectRatio = m_windowWidth / m_windowHeight;

					// 現在のノード内（インデント込み）の利用可能な横幅を取得
					float _availableWidth = ImGui::GetContentRegionAvail().x;

					// 横幅をウィンドウいっぱいにし、アスペクト比から高さを逆算
					float _drawWidth = _availableWidth;
					float _drawHeight = _drawWidth / _aspectRatio;

					// テクスチャ描画
					auto _gpuHandle = D3D12::DescriptorHeapManager::Instance().GetImGuiSRVGPUHandle(_upTempTex->GetImGuiSRV());
					Helper::DrawSRVView(_gpuHandle, static_cast<UINT>(_drawWidth), static_cast<UINT>(_drawHeight));

					ImGui::TreePop();
				}
			}
		}
		ImGui::EndChild();
	}
}