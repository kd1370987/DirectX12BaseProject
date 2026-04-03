#include "RenderPass.h"

namespace Engine::Graphics
{
	void RenderPass::Init(RenderGraph* a_graph, ShaderManager* a_pShaderMana, RootSignatureManager* a_pRootSigMana, Engine::D3D12::GraphicsPSOManager* a_pPSOMana)
	{
		m_passDesc = {};

		m_pRenderGraph = a_graph;
		m_pShaderMana = a_pShaderMana;
		m_pRootSigMana = a_pRootSigMana;
		m_pPSOMana = a_pPSOMana;

		CreatePass();
	}
}