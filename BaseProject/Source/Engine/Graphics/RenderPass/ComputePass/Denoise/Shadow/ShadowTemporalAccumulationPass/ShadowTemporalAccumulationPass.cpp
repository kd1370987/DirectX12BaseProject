#include "ShadowTemporalAccumulationPass.h"

#include "Engine/Graphics/RenderGraph/RenderGraph.h"
#include "Engine/Graphics/RenderGraph/RGPassBuilder/RGPassBuilder.h"
#include "Engine/Graphics/GraphicEngine.h"
#include "Engine/Graphics/RenderContext/RenderContext.h"
#include "Engine/D3D12/PipelineStateManager/PipelineStateManager.h"
#include "Engine/D3D12/D3DObject/CommandList/CommandList.h"
#include "Engine/Option/OptionManager.h"

namespace Engine::Graphics
{
	void AddShadowTemporalAccumulationPass(D3D12::PipelineStateManager* a_pPSOManager, RenderGraph& a_rg, const EDrawPhase& a_phase)
	{
		// ランタイムデータ
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

		// ノード作成
		RenderPassNode _node = {};
		_node.name = "ShadowTADenoisePass";
		RGComputePassBuilder _cpBuilder(&_node,&a_rg);

		// ルートシグネチャセット
		_spPassData->pRootSig = _cpBuilder.SetRootSignature(
			a_pPSOManager, "Asset/Shader/Compute/Denoise/Shadow/ShadowTemporalAccumullationShader.cso"
		);

		// シェーダーセット
		_cpBuilder.SetShader(
			"Asset/Shader/Compute/Denoise/Shadow/ShadowTemporalAccumullationShader.cso",
			"ShadowTemporalAccumullationShader",
			_spPassData->csIndex
		);

		// 依存関係構築
		_cpBuilder.Read("RayShadow", AccessType::SRV, LoadOp::Load, StoreOp::Store);
		_cpBuilder.Read("GBufferVelocity", AccessType::SRV, LoadOp::Load, StoreOp::Store);
		_cpBuilder.Read("AffterDLShadowTempAccumu", AccessType::SRV, LoadOp::Load, StoreOp::Store);
		_cpBuilder.Read("Depth", AccessType::SRV, LoadOp::Load, StoreOp::Store);
		_cpBuilder.Read("GBufferNormal", AccessType::SRV, LoadOp::Load, StoreOp::Store);
		_cpBuilder.Read("PrevDepth", AccessType::SRV, LoadOp::Load, StoreOp::Store);
		_cpBuilder.Read("PrevNormal", AccessType::SRV, LoadOp::Load, StoreOp::Store);

		_cpBuilder.Write("AffterDLShadowTempAccumu", AccessType::UAV, LoadOp::Load, StoreOp::Store);

		// PSO作成
		_cpBuilder.ResolveAndCompile(a_pPSOManager);

		// 実行時関数作成
		_node.executeFunc = [_spPassData](GraphicsEngine* a_pGE, RenderContext* a_pCtx, uint8_t a_passIndex)
			{
				Editor::MainEditor::Instance().StartWatch("ShadowTADenoisePass");

				// オプション取得
				const auto& _winOp = Option::OptionManager::GetInstance().GetInstance().GetWindowOption();

				auto* _pCmd = a_pCtx->GetCurrentCmdList();
				_pCmd->SetComputeRootSignature(_spPassData->pRootSig);
				auto* _pPSO = _spPassData->pPSOManager->GetPSO(_spPassData->csIndex);
				_pCmd->SetPipelineState(_pPSO);

				// 定数バッファ
				struct GITAOp
				{
					float phiDepth;
					float phiNormal;
					float blendRate;
				};
				GITAOp _op = {};
				const auto& _giOp = Option::OptionManager::GetInstance().GetGIOption();
				_op.phiDepth = _giOp.TAphiDepth;
				_op.phiNormal = _giOp.TAphiNormal;
				_op.blendRate = _giOp.TAblendRate;
				a_pCtx->BindCB()->BindAndAttachDataComputeRootCBV(
					_pCmd->NGet(),
					0,
					_op
				);

				a_pCtx->BindHeap();
				std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> _cpuVec = {
					_spPassData->pRG->GetSRVCPU("RayShadow"),
					_spPassData->pRG->GetSRVCPU("GBufferVelocity"),
					_spPassData->pRG->GetSRVCPU("AffterDLShadowTempAccumu"),
					_spPassData->pRG->GetSRVCPU("Depth"),
					_spPassData->pRG->GetSRVCPU("GBufferNormal"),
					_spPassData->pRG->GetSRVCPU("PrevDepth"),
					_spPassData->pRG->GetSRVCPU("PrevNormal")
				};
				a_pCtx->ComputeBindSRV(1, _cpuVec);

				a_pCtx->BindUAV(2, _spPassData->pRG->GetUAVCPU("AffterDLShadowTempAccumu"));
				// 実行
				UINT _countX = _winOp.windowWidth / 8;
				UINT _countY = _winOp.windowHegiht / 8;
				a_pCtx->Dispatch(_countX, _countY, 1);


				Editor::MainEditor::Instance().EndWatch("ShadowTADenoisePass");
			};

		// パスにノードを追加
		a_rg.AddPassNode(a_phase, _node);
	}
}