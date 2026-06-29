#include "SceneView.h"

#include "EditorCamera/EditorCamera.h"

#include "../../MainEngine.h"

#include "../../D3D12/DescriptorHeapManager/DescriptorHeapManager.h"

#include "../../Graphics/GraphicEngine.h"
#include "../../Graphics/RenderGraph/RenderGraph.h"

#include "../../Resource/Manager/ResourceManager/ResourceManager.h"

namespace Engine::Editor
{
	void Engine::Editor::SceneView::Init()
	{
		m_upEditorCamera = std::make_unique<EditorCamera>();
		m_upEditorCamera->Init(1280,720);
	}
	void SceneView::Update()
	{
		m_upEditorCamera->Update();
	}
	const EditorCamera* SceneView::GetEditorCamera()
	{
		if (!m_upEditorCamera) return nullptr;

		return m_upEditorCamera.get();
	}
	void SceneView::Draw()
	{
		if (ImGui::Begin("SceneView"))
		{
			ImVec2 _windowPos = ImGui::GetCursorScreenPos(); // ウィンドウの左上（タブなどを除いた純粋な描画領域のスタート位置）
			ImVec2 _windowSize = ImGui::GetContentRegionAvail(); // 残りのサイズ

			// 現在の最終出力テクスチャを取得
			auto* _pGE = MainEngine::Instance().RefGraphicsEngine();
			if (!_pGE)
			{
				ImGui::End();
				return;
			}
			auto* _pRG = _pGE->RefRenderGraph();
			if (!_pRG)
			{
				ImGui::End();
				return;
			}
			auto _texHandle = _pRG->GetTexHandle("AffterTAAColor");
			const auto* _pTex = Resource::ResourceManager::Instance().Get(_texHandle);
			if(!_pTex)
			{
				ImGui::End();
				return;
			}

			// テクスチャの描画 : 実際に描画した範囲も取得
			auto _gpuHandle = D3D12::DescriptorHeapManager::Instance().GetImGuiSRVGPUHandle(_pTex->GetImGuiSRV());
			ImVec2 _actualRenderSize = Helper::DrawSRVView(_gpuHandle, static_cast<UINT>(1980), static_cast<UINT>(1080));

			// 現在のウィンドウの描画リストにギズモを追加するよう指示
			ImGuizmo::SetDrawlist();

			// ギズモの操作・描画領域を、テクスチャの領域にぴったり合わせる
			ImGuizmo::SetRect(_windowPos.x, _windowPos.y, _actualRenderSize.x, _actualRenderSize.y);
		}
		ImGui::End();
	}
}
