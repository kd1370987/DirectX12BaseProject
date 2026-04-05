#include "DeferredLightingPass.h"

#include "Engine/Resource/Manager/ShaderManager/ShaderManager.h"
#include "Engine/D3D12/RootSignatureManager/RootSignatureManager.h"
#include "Engine/D3D12/PSOManager/GraphicsPSOManager/GraphicsPSOManager.h"
#include "Engine/Graphics/RenderGraph/RenderGraph.h"
#include "Engine/Graphics/RenderContext/RenderContext.h"

#include "Engine/D3D12/D3D12Wrapper/D3D12Wrapper.h"
namespace Engine::Graphics
{
	void DeferredLightingPass::Excute(RenderContext* a_pCtx)
	{
		Begin(a_pCtx);

		std::vector<D3D12_GPU_DESCRIPTOR_HANDLE> _gpuVec = {};

		_gpuVec = {
			m_pRenderGraph->GetGPUHandle("GBufferAlbedo"),
			m_pRenderGraph->GetGPUHandle("GBufferNormal"),
			m_pRenderGraph->GetGPUHandle("GBufferMaterial"),
			m_pRenderGraph->GetGPUHandle("GBufferEmissiv"),
			m_pRenderGraph->GetGPUHandle("Depth")
		};

		a_pCtx->BindSRV(
			RootSigSemantic::PostScreenSRV,
			_gpuVec
		);

		auto* _pCmdList = D3D12Wrapper::Instance().GetCommandList();

		// 描画
		_pCmdList->DrawInstanced(
			3, 1, 0, 0
		);

		End(a_pCtx);
	}

	void DeferredLightingPass::CreatePass()
	{

		SetName("DeferredLighting");

		SetVS("Asset/Shader/Source/DeferredLightingShader/DeferredLightingVS.cso");
		SetPS("Asset/Shader/Source/DeferredLightingShader/DeferredLightingPS.cso");
		SetRootSig("DeferredLighting");

		m_psoDesc.DepthEnable(false);
		m_psoDesc.DepthWriteMask(false);

		AddRead("Depth", AccessType::SRV, LoadOp::Load, StoreOp::Store);
		AddRead("GBufferAlbedo", AccessType::SRV, LoadOp::Load, StoreOp::Store);
		AddRead("GBufferNormal", AccessType::SRV, LoadOp::Load, StoreOp::Store);
		AddRead("GBufferMaterial", AccessType::SRV, LoadOp::Load, StoreOp::Store);
		AddRead("GBufferEmissiv", AccessType::SRV, LoadOp::Load, StoreOp::Store);

		AddWrite("QuadTexture", AccessType::RTV, LoadOp::Clear, StoreOp::Store);
	}
}