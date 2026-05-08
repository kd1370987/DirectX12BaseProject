#include "BaseRenderPass.h"
#include "Engine/D3D12/RootSignatureManager/RootSignatureManager.h"
#include "Engine/D3D12/PSOManager/GraphicsPSOManager/GraphicsPSOManager.h"
#include "Engine/Graphics/RenderGraph/RenderGraph.h"

namespace Engine::Graphics
{
	void BaseRenderPass::Init(const PassInitDesc& a_initDesc)
	{
		m_pRG = a_initDesc.pRG;
		m_pRootSigMana = a_initDesc.pRootSigMana;
		m_pPSOMana = a_initDesc.pPSOMana;
		
		m_pPipelineStateManager = a_initDesc.pPipelineStateManager;

		CreatePass();
	}

	void BaseRenderPass::SetPassName(const std::string& a_name)
	{
		m_name = a_name;
	}

	void BaseRenderPass::AddRead(const std::string& a_texName, AccessType a_type, LoadOp a_loadOp, StoreOp a_storeOp)
	{
		// 依存先の追加
		auto _resID = m_pRG->Read(a_texName,AccessType::SRV);
		m_read.push_back(_resID);

		// リソースの使い方指定
		m_resourceAccessVec.push_back({ _resID,a_type,a_loadOp,a_storeOp });
	}

	void BaseRenderPass::AddRead(const std::string& a_texName)
	{
		// 依存先の追加
		auto _resID = m_pRG->Read(a_texName, AccessType::SRV);
		m_read.push_back(_resID);
	}

	Engine::Resource::ID BaseRenderPass::AddWrite(const std::string& a_texName, AccessType a_type, LoadOp a_loadOp, StoreOp a_storeOp)
	{
		// 依存先
		auto _resID = m_pRG->Write(a_texName, a_type);
		m_write.push_back(_resID);

		// リソースの使い方指定
		m_resourceAccessVec.push_back({ _resID ,a_type,a_loadOp,a_storeOp });
		return _resID;
	}
}