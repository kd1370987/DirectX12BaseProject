#include "ComputePass.h"

#include "Engine/D3D12/PipelineStateManager/PipelineStateManager.h"
#include "Engine/Resource/Loader/Shader/ShaderLoader.h"
#include "Engine/Resource/Manager/ResourceManager/ResourceManager.h"

#include "../../GraphicEngine.h"
#include "../../RenderContext/RenderContext.h"
#include "../../../D3D12/D3DObject/CommandList/CommandList.h"
namespace Engine::Graphics
{
	void ComputePass::Init(const PassInitDesc& a_initDesc)
	{
		BaseRenderPass::Init(a_initDesc);

		m_pPipelineStateManager->Request(m_csPSODesc);
		m_csPSOHandle = m_pPipelineStateManager->RequestHandle(m_csPSODesc);
	}
	void ComputePass::SetName(const std::string& a_name)
	{
		m_name = a_name;
		m_csPSODesc.SetName(a_name);
	}
	void ComputePass::SetShader(const std::string& a_filePath)
	{
		auto _csHandle = Resource::ShaderLoader::Request(a_filePath);
		auto* _cs = Resource::ResourceManager::Instance().Ref(_csHandle);
		m_csPSODesc.desc.CS.pShaderBytecode = _cs->Get()->GetBufferPointer();
		m_csPSODesc.desc.CS.BytecodeLength = _cs->Get()->GetBufferSize();


		m_pRootSig = m_pPipelineStateManager->Request(a_filePath);
		if (!m_pRootSig)
		{
			assert(0 && "ルートシグネチャが設定されていません");
			return;
		}
		m_csPSODesc.SetRootSignature(m_pRootSig);
	}
	void ComputePass::SetPSO(RenderContext* a_pCtx)
	{
		auto* _pCmd = a_pCtx->GetCurrentCmdList();
		_pCmd->SetComputeRootSignature(m_pRootSig);
		auto* _pPSO = m_pPipelineStateManager->GetPSO(m_csPSOHandle);
		_pCmd->SetPipelineState(_pPSO);
	}
}