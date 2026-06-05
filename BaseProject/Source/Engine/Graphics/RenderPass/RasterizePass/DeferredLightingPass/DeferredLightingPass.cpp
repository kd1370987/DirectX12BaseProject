#include "DeferredLightingPass.h"

#include "Engine/Graphics/RenderGraph/RenderGraph.h"
#include "Engine/Graphics/RenderGraph/RGPassBuilder/RGPassBuilder.h"
#include "Engine/Graphics/GraphicEngine.h"

#include "Engine/Graphics/RenderContext/RenderContext.h"
#include "Engine/D3D12/PipelineStateManager/PipelineStateManager.h"

#include "Engine/D3D12/D3D12Wrapper/D3D12Wrapper.h"

#include "Engine/D3D12/CBAllocater/CBAllocater.h"
#include "Engine/D3D12/D3DObject/CommandList/CommandList.h"

namespace Engine::Graphics
{
	void AddDeferredLightingPass(D3D12::PipelineStateManager* a_pPSOManager, RenderGraph& a_rg, const EDrawPhase& a_phase)
	{
		// ランタイム用データ
		struct RuntimeData
		{
			ID3D12RootSignature* pRootSig;
			uint8_t staticIndex;
			D3D12::PipelineStateManager* pPSOManager;
			RenderGraph* pRG;
		};
		auto _spPassData = std::make_shared<RuntimeData>();
		_spPassData->pPSOManager = a_pPSOManager;
		_spPassData->pRG = &a_rg;

		// ノード・ビルダー作成
		RenderPassNode _node = {};
		_node.name = "DeferredLighting";
		RGRasterPassBuilder _rpBuilder(&_node, &a_rg);

		// パス共通設定
		_rpBuilder.SetRootSignature(a_pPSOManager, "Asset/Shader/Source/DeferredLightingShader/DeferredLightingVS.cso");
		_spPassData->pRootSig = a_pPSOManager->Request("Asset/Shader/Source/DeferredLightingShader/DeferredLightingVS.cso");

		// 依存関係構築
		_rpBuilder.Read("GBufferAlbedo", AccessType::SRV, LoadOp::Load, StoreOp::Store);
		_rpBuilder.Read("GBufferNormal", AccessType::SRV, LoadOp::Load, StoreOp::Store);
		_rpBuilder.Read("GBufferMaterial", AccessType::SRV, LoadOp::Load, StoreOp::Store);
		_rpBuilder.Read("GBufferEmissiv", AccessType::SRV, LoadOp::Load, StoreOp::Store);
		_rpBuilder.Read("Depth", AccessType::SRV, LoadOp::Load, StoreOp::Store);
		_rpBuilder.Read("RayShadow", AccessType::SRV, LoadOp::Load, StoreOp::Store);
		_rpBuilder.Read("FinalGI", AccessType::SRV, LoadOp::Load, StoreOp::Store);

		_rpBuilder.Write("QuadTexture", AccessType::RTV, LoadOp::Clear, StoreOp::Store);

		// PSO構築
		auto& _sPso = _rpBuilder.CreatePSODesc("DeferredLighting", _spPassData->staticIndex);
		
		// SetVS には InputLayout の指定が必要ですが、元コードに無かったため空を渡すか StaticLayout にします
		// 元コード: SetVS(ERenderType::Static,"Asset/Shader/Source/DeferredLightingShader/DeferredLightingVS.cso");
		_rpBuilder.SetVS(_sPso, "Asset/Shader/Source/DeferredLightingShader/DeferredLightingVS.cso", D3D12::Input::StaticLayout);
		_rpBuilder.SetPS(_sPso, "Asset/Shader/Source/DeferredLightingShader/DeferredLightingPS.cso");
		
		_sPso.DepthEnable(false);
		_sPso.DepthWriteMask(false);

		// コンパイル
		_rpBuilder.ResolveAndCompile(a_pPSOManager);

		// ピンポンリソース用
		std::string _tempA = "DeferedLighting_A";
		std::string _tempB = "DeferedLighting_B";

		// 実行関数
		_node.executeFunc = [_spPassData](GraphicsEngine* a_pGE, RenderContext* a_pCtx, uint8_t a_passIndex)
		{
			Editor::MainEditor::Instance().StartWatch("DeferredLighting");
			a_pCtx->BindHeap();
			a_pCtx->SetGraphicsRootSignature(_spPassData->pRootSig);

			CameraData _cbCam = a_pGE->GetCameraData();
			auto* _pCmd = a_pCtx->GetCurrentCmdList();
			a_pCtx->BindCB()->BindAndAttachDataRootCBV<CameraData>(_pCmd->NGet(), 0, _cbCam);
			
			auto* _pPSO = _spPassData->pPSOManager->GetPSO(_spPassData->staticIndex);
			a_pCtx->SetGraphicPSO(_pPSO);

			const AmbientData& _amib = a_pGE->GetAmbientData();
			a_pCtx->BindCB()->BindAndAttachDataRootCBV<AmbientData>(_pCmd->NGet(), 1, _amib);
			std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> _gpuVec = {
				_spPassData->pRG->GetCPUHandle("GBufferAlbedo"),
				_spPassData->pRG->GetCPUHandle("GBufferNormal"),
				_spPassData->pRG->GetCPUHandle("GBufferMaterial"),
				_spPassData->pRG->GetCPUHandle("GBufferEmissiv"),
				_spPassData->pRG->GetCPUHandle("Depth"),
				_spPassData->pRG->GetCPUHandle("RayShadow"),
				_spPassData->pRG->GetCPUHandle("FinalGI")
			};

			a_pCtx->BindSRV(2,_gpuVec);

			auto* _pCmdList = Engine::D3D12::D3D12Wrapper::Instance().GetCommandList();
			_pCmdList->DrawInstanced(3, 1, 0, 0);

			Editor::MainEditor::Instance().EndWatch("DeferredLighting");
		};

		// パス登録
		a_rg.AddPassNode(a_phase, _node);
	}
}