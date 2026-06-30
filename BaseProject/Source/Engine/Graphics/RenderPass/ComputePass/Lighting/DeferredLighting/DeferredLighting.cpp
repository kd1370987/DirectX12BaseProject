#include "DeferredLighting.h"

#include "Engine/Graphics/RenderPassRegistry/RenderPassRegistry.h"
#include "Engine/Graphics/RenderGraph/RenderGraph.h"
#include "Engine/Graphics/RenderGraph/RGPassBuilder/RGPassBuilder.h"
#include "Engine/Graphics/GraphicEngine.h"

#include "Engine/Graphics/RenderContext/RenderContext.h"
#include "Engine/D3D12/PipelineStateManager/PipelineStateManager.h"

#include "Engine/D3D12/D3D12Wrapper/D3D12Wrapper.h"

#include "Engine/D3D12/CBAllocater/CBAllocater.h"

#include "../../../../../Option/OptionManager.h"

namespace Engine::Graphics
{
	void AddDeferredLighting(D3D12::PipelineStateManager* a_pPSOManager, RenderGraph& a_rg, const EDrawPhase& a_phase)
	{
		// ランタイム用データ
		struct RuntimeData
		{
			ID3D12RootSignature* pRootSig;
			uint8_t csIndex;
			D3D12::PipelineStateManager* pPSOManager;
			RenderGraph* pRG;
		};
		auto _spPassData = std::make_shared<RuntimeData>();
		_spPassData->pPSOManager = a_pPSOManager;
		_spPassData->pRG = &a_rg;

		// ノード・ビルダー作成
		RenderPassNode _node = {};
		_node.name = "DeferredLighting";
		RGComputePassBuilder _rpBuilder(&_node);

		// ルートシグネチャ
		_spPassData->pRootSig = _rpBuilder.SetRootSignature(
			a_pPSOManager, 
			"Asset/Shader/Compute/Lighting/DeferredLighting/DeferredLightingShader.cso"
			);
		// シェーダー
		_rpBuilder.SetShader(
			"Asset/Shader/Compute/Lighting/DeferredLighting/DeferredLightingShader.cso",
			"DeferredLightingShader", 
			_spPassData->csIndex
		);

		// 依存関係構築
		_rpBuilder.ReadSRV("GBufferAlbedo");
		_rpBuilder.ReadSRV("GBufferNormal");
		_rpBuilder.ReadSRV("GBufferMaterial");
		_rpBuilder.ReadSRV("GBufferEmissiv");
		_rpBuilder.ReadSRV("Depth");
		_rpBuilder.ReadSRV("AffterDLShadowTempAccumu");
		_rpBuilder.ReadSRV("FinalGI");

		_rpBuilder.WriteUAV("AffterLighting", DXGI_FORMAT_R8G8B8A8_UNORM, LoadOp::Clear, StoreOp::Store);

		// コンパイル
		_rpBuilder.ResolveAndCompile(a_pPSOManager);

		// 実行関数
		_node.executeFunc = [_spPassData](GraphicsEngine* a_pGE, RenderContext* a_pCtx, uint8_t a_passIndex)
		{
			// オプション取得
			const auto& _winOp = Option::OptionManager::GetInstance().GetInstance().GetWindowOption();
			// ヒープとルートシグネチャ、PSOをセット
			auto* _pPSO = _spPassData->pPSOManager->GetPSO(_spPassData->csIndex);
			a_pCtx->BindHeap();
			a_pCtx->SetComputeRootSignature(_spPassData->pRootSig);
			a_pCtx->SetComputePSO(_pPSO);

			// カメラセット
			CameraData _cbCam = a_pGE->GetCameraData();
			auto* _pCmd = a_pCtx->GetCurrentCmdList();
			a_pCtx->BindCB()->BindAndAttachDataComputeRootCBV<CameraData>(
				_pCmd,
				0, 
				_cbCam
			);

			// アンビエントカラー
			const AmbientData& _amib = a_pGE->GetAmbientData();
			a_pCtx->BindCB()->BindAndAttachDataComputeRootCBV<AmbientData>(
				_pCmd, 
				1,
				_amib
			);

			// 参照SRV
			std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> _gpuVec = {
				_spPassData->pRG->GetCPUHandle("GBufferAlbedo"),
				_spPassData->pRG->GetCPUHandle("GBufferNormal"),
				_spPassData->pRG->GetCPUHandle("GBufferMaterial"),
				_spPassData->pRG->GetCPUHandle("GBufferEmissiv"),
				_spPassData->pRG->GetCPUHandle("Depth"),
				_spPassData->pRG->GetCPUHandle("AffterDLShadowTempAccumu"),
				_spPassData->pRG->GetCPUHandle("FinalGI")
			};
			a_pCtx->ComputeBindSRV(2, _gpuVec);

			// 出力テクスチャ設定
			a_pCtx->BindUAV(3, _spPassData->pRG->GetUAVCPU("AffterLighting"));

			// 実行
			UINT _countX = _winOp.windowWidth / 8;
			UINT _countY = _winOp.windowHegiht / 8;
			a_pCtx->Dispatch(_countX, _countY, 1);
		};

		// パス登録
		a_rg.AddPassNode(a_phase, _node);
	}
	void AddDeferredLighting(D3D12::PipelineStateManager* a_pPSOManager, RenderPassRegistry* a_pRegistry, const EDrawPhase& a_phase)
	{
		// ランタイム用データ
		struct RuntimeData
		{
			ID3D12RootSignature* pRootSig;
			uint8_t csIndex;
			D3D12::PipelineStateManager* pPSOManager;
			
		};
		auto _spPassData = std::make_shared<RuntimeData>();
		_spPassData->pPSOManager = a_pPSOManager;
		

		// ノード・ビルダー作成
		RenderPassNode _node = {};
		_node.name = "DeferredLighting";
		_node.phase = a_phase;
		RGComputePassBuilder _rpBuilder(&_node);

		// ルートシグネチャ
		_spPassData->pRootSig = _rpBuilder.SetRootSignature(
			a_pPSOManager, 
			"Asset/Shader/Compute/Lighting/DeferredLighting/DeferredLightingShader.cso"
			);
		// シェーダー
		_rpBuilder.SetShader(
			"Asset/Shader/Compute/Lighting/DeferredLighting/DeferredLightingShader.cso",
			"DeferredLightingShader", 
			_spPassData->csIndex
		);

		// 依存関係構築
		_rpBuilder.ReadSRV("GBufferAlbedo");
		_rpBuilder.ReadSRV("GBufferNormal");
		_rpBuilder.ReadSRV("GBufferMaterial");
		_rpBuilder.ReadSRV("GBufferEmissiv");
		_rpBuilder.ReadSRV("Depth");
		_rpBuilder.ReadSRV("AffterDLShadowTempAccumu");
		_rpBuilder.ReadSRV("FinalGI");

		_rpBuilder.WriteUAV("AffterLighting", DXGI_FORMAT_R8G8B8A8_UNORM, LoadOp::Clear, StoreOp::Store);

		// コンパイル
		_rpBuilder.ResolveAndCompile(a_pPSOManager);

		// 実行関数
		_node.executeFunc = [_spPassData](GraphicsEngine* a_pGE, RenderContext* a_pCtx, uint8_t a_passIndex)
		{
			// オプション取得
			const auto& _winOp = Option::OptionManager::GetInstance().GetInstance().GetWindowOption();
			// ヒープとルートシグネチャ、PSOをセット
			auto* _pPSO = _spPassData->pPSOManager->GetPSO(_spPassData->csIndex);
			a_pCtx->BindHeap();
			a_pCtx->SetComputeRootSignature(_spPassData->pRootSig);
			a_pCtx->SetComputePSO(_pPSO);

			// カメラセット
			CameraData _cbCam = a_pGE->GetCameraData();
			auto* _pCmd = a_pCtx->GetCurrentCmdList();
			a_pCtx->BindCB()->BindAndAttachDataComputeRootCBV<CameraData>(
				_pCmd,
				0, 
				_cbCam
			);

			// アンビエントカラー
			const AmbientData& _amib = a_pGE->GetAmbientData();
			a_pCtx->BindCB()->BindAndAttachDataComputeRootCBV<AmbientData>(
				_pCmd, 
				1,
				_amib
			);

			// 参照SRV
			std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> _gpuVec = {
				a_pGE->RefRenderGraph()->GetCPUHandle("GBufferAlbedo"),
				a_pGE->RefRenderGraph()->GetCPUHandle("GBufferNormal"),
				a_pGE->RefRenderGraph()->GetCPUHandle("GBufferMaterial"),
				a_pGE->RefRenderGraph()->GetCPUHandle("GBufferEmissiv"),
				a_pGE->RefRenderGraph()->GetCPUHandle("Depth"),
				a_pGE->RefRenderGraph()->GetCPUHandle("AffterDLShadowTempAccumu"),
				a_pGE->RefRenderGraph()->GetCPUHandle("FinalGI")
			};
			a_pCtx->ComputeBindSRV(2, _gpuVec);

			// 出力テクスチャ設定
			a_pCtx->BindUAV(3, a_pGE->RefRenderGraph()->GetUAVCPU("AffterLighting"));

			// 実行
			UINT _countX = _winOp.windowWidth / 8;
			UINT _countY = _winOp.windowHegiht / 8;
			a_pCtx->Dispatch(_countX, _countY, 1);
		};

		// パス登録
		_node.phase = a_phase;
		a_pRegistry->RegisterPass(_node);
	}
}