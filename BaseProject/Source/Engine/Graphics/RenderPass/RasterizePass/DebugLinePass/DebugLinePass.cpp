#include "DebugLinePass.h"
#include "Engine/Graphics/RenderGraph/RenderGraph.h"
#include "Engine/Graphics/RenderContext/RenderContext.h"

#include "../../../../D3D12/Builder/RootSignatureBuilder/RootSignatureBuilder.h"
#include "../../../../D3D12/PipelineStateManager/PipelineStateManager.h"
namespace Engine::Graphics
{
	void DebugLinePass::Excute(RenderContext* a_pCtx)
	{
		Editor::MainEditor::Instance().StartWatch("DebugLineDraw");
		Begine(a_pCtx);
		a_pCtx->BindCameraCB();

		a_pCtx->SetGraphicPSO(m_pPsoVec[0].first);
		a_pCtx->SetPrimitive(D3D_PRIMITIVE_TOPOLOGY_LINELIST);

		auto _draws = a_pCtx->GetItemVec(RenderQueueType::Debug);
		if (_draws.size() == 0) return;
		for (auto& _item : _draws)
		{
			a_pCtx->BindMesh(1,_item.pMesh, _item.worldMat);

			a_pCtx->Draw(_item.pMesh, _item.subIdx);
		}

		a_pCtx->ShapeDraw();

		End(a_pCtx);
		Editor::MainEditor::Instance().EndWatch("DebugLineDraw");
	}

	void DebugLinePass::CreatePass()
	{
		SetPassName("DebugLine");
		// ルートシグネチャの作成
		D3D12::RootSignatureDesc _desc = {};
		_desc.AddRoot(RootParameterType::RootCBV,0);
		_desc.AddRoot(RootParameterType::RootCBV,1);
		m_pRootSig = m_pPipelineStateManager->Request(_desc);

		// PSOの作成
		auto& _sPso = AddPSODesc(ERenderType::Static,RenderQueueType::Debug);
		_sPso.SetName("DebugLinePass");

		SetInputLayout(ERenderType::Static,D3D12::Input::gPosOnryLayout);
		SetVS(ERenderType::Static,"Asset/Shader/Source/DebugLineShader/DebugLineVS.cso");
		SetPS("Asset/Shader/Source/DebugLineShader/DebugLinePS.cso");

		_sPso.FillMode(D3D12_FILL_MODE_WIREFRAME);
		_sPso.CullMode(D3D12_CULL_MODE_NONE);

		_sPso.DepthFunc(D3D12_COMPARISON_FUNC_LESS_EQUAL);

		_sPso.desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;

		AddRead("MainColor");
		AddRead("Depth", AccessType::Depth_Write, LoadOp::Load, StoreOp::Store);

		AddWrite("MainColor", AccessType::RTV, LoadOp::Clear, StoreOp::Store);
	}
}