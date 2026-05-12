#include "DeferredLightingPass.h"

#include "Engine/D3D12/RootSignatureManager/RootSignatureManager.h"
#include "Engine/D3D12/PSOManager/GraphicsPSOManager/GraphicsPSOManager.h"
#include "Engine/Graphics/RenderGraph/RenderGraph.h"
#include "Engine/Graphics/RenderContext/RenderContext.h"
#include "../../../../D3D12/PipelineStateManager/PipelineStateManager.h"

#include "Engine/D3D12/D3D12Wrapper/D3D12Wrapper.h"
namespace Engine::Graphics
{
	void DeferredLightingPass::Excute(RenderContext* a_pCtx)
	{
		Begine(a_pCtx);
		a_pCtx->BindCameraCB();

		a_pCtx->SetGraphicPSO(m_pPsoVec[0].first);

		a_pCtx->BindAmbientCB();

		std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> _gpuVec = {};

		_gpuVec = {
			m_pRG->GetCPUHandle("GBufferAlbedo"),
			m_pRG->GetCPUHandle("GBufferNormal"),
			m_pRG->GetCPUHandle("GBufferMaterial"),
			m_pRG->GetCPUHandle("GBufferEmissiv"),
			m_pRG->GetCPUHandle("Depth")
		};

		a_pCtx->BindSRV(2,_gpuVec);

		auto* _pCmdList = Engine::D3D12::D3D12Wrapper::Instance().GetCommandList();

		// 描画
		_pCmdList->DrawInstanced(3, 1, 0, 0);

		End(a_pCtx);
	}

	void DeferredLightingPass::CreatePass()
	{
		auto& _sPso = AddPSODesc(ERenderType::Static, RenderQueueType::Bloom);
		_sPso.SetName("DeferredLighting");

		SetVS(ERenderType::Static,"Asset/Shader/Source/DeferredLightingShader/DeferredLightingVS.cso");
		SetPS("Asset/Shader/Source/DeferredLightingShader/DeferredLightingPS.cso");

		_sPso.DepthEnable(false);
		_sPso.DepthWriteMask(false);

		AddRead("GBufferAlbedo", AccessType::SRV, LoadOp::Load, StoreOp::Store);
		AddRead("GBufferNormal", AccessType::SRV, LoadOp::Load, StoreOp::Store);
		AddRead("GBufferMaterial", AccessType::SRV, LoadOp::Load, StoreOp::Store);
		AddRead("GBufferEmissiv", AccessType::SRV, LoadOp::Load, StoreOp::Store);
		AddRead("Depth", AccessType::SRV, LoadOp::Load, StoreOp::Store);

		AddWrite("QuadTexture", AccessType::RTV, LoadOp::Clear, StoreOp::Store);
	}
}