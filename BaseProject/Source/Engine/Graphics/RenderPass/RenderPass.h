#pragma once

class ShaderManager;
class RootSignatureManager;
namespace Engine::D3D12
{
	class GraphicsPSOManager;
}

namespace Engine::Graphics
{
	class RenderGraph;
	class RenderContext;

	class RenderPass
	{
	public:

		RenderPass()
		{
			m_passDesc = {};
		}
		virtual ~RenderPass() = default;

		void Init(
			RenderGraph* a_graph,
			ShaderManager* a_pShaderMana,
			RootSignatureManager* a_pRootSigMana,
			Engine::D3D12::GraphicsPSOManager* a_pPSOMana
		);

		const PassDesc& GetDesc() const
		{
			return m_passDesc;
		}

		virtual void Excute(RenderContext* a_ctx) = 0;

	protected:

		virtual void CreatePass() = 0;

		PassDesc m_passDesc = {};

		ShaderManager* m_pShaderMana = nullptr;
		RootSignatureManager* m_pRootSigMana = nullptr;
		Engine::D3D12::GraphicsPSOManager* m_pPSOMana = nullptr;
		RenderGraph* m_pRenderGraph = nullptr;
	};
}