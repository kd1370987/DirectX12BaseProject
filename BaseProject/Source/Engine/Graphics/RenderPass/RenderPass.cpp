#include "RenderPass.h"
#include "Engine/Resource/Manager/ShaderManager/ShaderManager.h"
#include "Engine/D3D12/RootSignatureManager/RootSignatureManager.h"
#include "Engine/D3D12/PSOManager/GraphicsPSOManager/GraphicsPSOManager.h"
#include "Engine/Graphics/RenderGraph/RenderGraph.h"

namespace Engine::Graphics
{
	void RenderPass::Init(RenderGraph* a_graph, Resource::ShaderManager* a_pShaderMana, RootSignatureManager* a_pRootSigMana, Engine::D3D12::GraphicsPSOManager* a_pPSOMana)
	{
		m_passDesc = {};

		m_pRenderGraph = a_graph;
		m_pShaderMana = a_pShaderMana;
		m_pRootSigMana = a_pRootSigMana;
		m_pPSOMana = a_pPSOMana;

		m_psoDesc = {};
		m_psoDesc.CullMode(D3D12_CULL_MODE_BACK);

		CreatePass();

		m_passDesc.psoID = m_pPSOMana->Request(m_psoDesc);
	}
	void RenderPass::SetName(const std::string& a_name)
	{
		m_passDesc.name = a_name;
		m_psoDesc.SetName(a_name);
	}
	void RenderPass::SetInputLayout(const D3D12_INPUT_LAYOUT_DESC& a_desc)
	{
		m_psoDesc.SetInputLayout(a_desc);
	}
	void RenderPass::SetVS(const std::string& a_pathName)
	{
		Resource::Handle<Resource::Shader> _vsHandle =
			m_pShaderMana->Request(a_pathName);
		m_psoDesc.SetVS(m_pShaderMana->GetByteCode(_vsHandle));
	}
	void RenderPass::SetPS(const std::string & a_pathName)
	{
		Resource::Handle<Resource::Shader> _psHandle =
			m_pShaderMana->Request(a_pathName);
		m_psoDesc.SetPS(m_pShaderMana->GetByteCode(_psHandle));
	}
	void RenderPass::SetRootSig(const std::string & a_pathName)
	{
		Engine::Resource::ID _rootSigID = m_pRootSigMana->GetID(a_pathName);
		m_psoDesc.SetRootSignature(m_pRootSigMana->NGet(_rootSigID));
		m_passDesc.rootSigID = _rootSigID;
	}
	void RenderPass::AddRead(const std::string& a_texName, AccessType a_type, LoadOp a_loadOp, StoreOp a_storeOp)
	{
		//auto _resID = m_pRenderGraph->GetID(a_texName);
		auto _resID = m_pRenderGraph->Read(a_texName, AccessType::SRV);
		m_passDesc.readResource.push_back(_resID);


		m_passDesc.resourceAccessVec.push_back({ _resID ,a_type,a_loadOp,a_storeOp });
	}
	void RenderPass::AddRead(const std::string& a_texName)
	{
		//auto _resID = m_pRenderGraph->GetID(a_texName);
		auto _resID = m_pRenderGraph->Read(a_texName,AccessType::SRV);

		m_passDesc.readResource.push_back(_resID);
	}
	void RenderPass::AddWrite(const std::string & a_texName, AccessType a_type, LoadOp a_loadOp, StoreOp a_storeOp)
	{
		//auto _resID = m_pRenderGraph->GetID(a_texName);
		auto _resID = m_pRenderGraph->Write(a_texName,a_type);
		m_passDesc.writeResource.push_back(_resID);

		//auto& _res = m_pRenderGraph->GetRGresource(_resID);
		auto _format = m_pRenderGraph->GetDXGIFormat(_resID);
		m_psoDesc.AddRenderTargetFormat(_format);

		m_passDesc.resourceAccessVec.push_back({ _resID ,a_type,a_loadOp,a_storeOp });
	}
}