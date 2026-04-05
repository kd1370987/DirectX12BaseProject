#pragma once

namespace Engine::Resource
{
	class ShaderManager;
}

class RootSignatureManager;
namespace Engine::D3D12
{
	class GraphicsPSOManager;
}

namespace Engine::Graphics
{
	class RenderGraph;
	class RenderContext;

	// 描画用パスクラス
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
			Resource::ShaderManager* a_pShaderMana,
			RootSignatureManager* a_pRootSigMana,
			Engine::D3D12::GraphicsPSOManager* a_pPSOMana
		);

		const PassDesc& GetDesc() const
		{
			return m_passDesc;
		}

		virtual void Excute(RenderContext* a_ctx) = 0;

	protected:

		// ビルドヘルパー
		void SetName(const std::string& a_name);

		void SetInputLayout(const D3D12_INPUT_LAYOUT_DESC& a_desc);
		void SetVS(const std::string& a_pathName);
		void SetPS(const std::string& a_pathName);
		void SetRootSig(const std::string& a_pathName);

		void AddRead(const std::string& a_texName, AccessType a_type, LoadOp a_loadOp, StoreOp a_storeOp);
		void AddRead(const std::string& a_texName);
		void AddWrite(const std::string& a_texName, AccessType a_type, LoadOp a_loadOp, StoreOp a_storeOp);

		virtual void CreatePass() = 0;

		PassDesc m_passDesc = {};

		D3D12::GraphicsPipelineDesc m_psoDesc = {};

		Resource::ShaderManager* m_pShaderMana = nullptr;
		RootSignatureManager* m_pRootSigMana = nullptr;
		Engine::D3D12::GraphicsPSOManager* m_pPSOMana = nullptr;
		RenderGraph* m_pRenderGraph = nullptr;
	};
}