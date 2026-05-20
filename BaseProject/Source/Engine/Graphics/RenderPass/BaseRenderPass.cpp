#include "BaseRenderPass.h"
#include "Engine/Graphics/RenderGraph/RenderGraph.h"

namespace Engine::Graphics
{
	void BaseRenderPass::Init(const PassInitDesc& a_initDesc)
	{
		m_pRG = a_initDesc.pRG;
		m_pPipelineStateManager = a_initDesc.pPipelineStateManager;

		CreatePass();
	}

	uint8_t BaseRenderPass::GetPSOIndex(const std::string& a_psoName) const
	{
		auto _it = m_psoIndexMap.find(a_psoName);
		if (_it != m_psoIndexMap.end())
		{
			return _it->second;
		}
		return 255;
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