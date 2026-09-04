#pragma once

#include "../IPanel.h"

namespace Engine::Graphics::Pipeline
{
	class RenderGraph;
	class VirtualResource;
}

namespace Engine::Editor
{
	//======================================================================================
	// レンダリングパイプラインが確保しているリソースの一覧
	//
	// 実行インスタンスはカメラごとにあるので、まずカメラを選んでから中身を見る
	//======================================================================================
	class RenderGraphResourceViewPanel : public IPanel
	{
	public:
		~RenderGraphResourceViewPanel() override = default;

		const char* GetName() const override { return "RenderGraphResourceView"; };
		void OnDrawImGui(EditorContext& a_editContext) override;

	private:

		// 生存区間(このリソースに触る最初と最後のパス)を出す
		void DrawLifetime(
			const Graphics::Pipeline::RenderGraph& a_graph, const Graphics::Pipeline::VirtualResource& a_resource);

		// リソース1本ぶんの絵を出す。履歴つきなら2枚並べる
		void DrawResourceImage(
			const Graphics::Pipeline::RenderGraph& a_graph, const Graphics::Pipeline::VirtualResource& a_resource);

	private:

		float m_windowWidth = 1920;
		float m_windowHeight = 1080;

		// どのカメラのグラフを見ているか。
		// カメラが減ると添字がずれるので、描くたびに範囲へ収め直す
		int m_selectedGraph = 0;
	};
}
