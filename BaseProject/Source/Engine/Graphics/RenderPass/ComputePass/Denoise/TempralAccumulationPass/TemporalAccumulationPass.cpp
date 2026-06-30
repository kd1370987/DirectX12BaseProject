#include "TemporalAccumulationPass.h"

#include "Engine/Graphics/RenderGraph/RenderGraph.h"
#include "Engine/Graphics/RenderGraph/RGPassBuilder/RGPassBuilder.h"
#include "Engine/Graphics/GraphicEngine.h"

#include "Engine/Graphics/RenderContext/RenderContext.h"
#include "Engine/D3D12/PipelineStateManager/PipelineStateManager.h"

#include "../../../../../Option/OptionManager.h"

namespace Engine::Graphics
{
	void AddTemporalAccumulationPass(D3D12::PipelineStateManager* a_pPSOManager, RenderGraph& a_rg, const EDrawPhase& a_phase)
	{
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

		RenderPassNode _node = {};
		_node.name = "TemporalAccumulationPass";
		RGComputePassBuilder _cpBuilder(&_node, &a_rg);

		_cpBuilder.SetRootSignature(a_pPSOManager, "Asset/Shader/Compute/TemporalAccumulationShader/TemporalAccumulationShader.cso");
		_spPassData->pRootSig = a_pPSOManager->Request("Asset/Shader/Compute/TemporalAccumulationShader/TemporalAccumulationShader.cso");

		_cpBuilder.SetShader("Asset/Shader/Compute/TemporalAccumulationShader/TemporalAccumulationShader.cso", "TemporalAccumulationPass", _spPassData->csIndex);

		_cpBuilder.ReadSRV("RayGI");
		_cpBuilder.ReadSRV("GBufferVelocity");
		_cpBuilder.ReadHistorySRV("DenoiseGI");
		_cpBuilder.ReadSRV("Depth");
		_cpBuilder.ReadSRV("GBufferNormal");
		_cpBuilder.ReadSRV("PrevDepth");
		_cpBuilder.ReadSRV("PrevNormal");

		_cpBuilder.WriteTemporalUAV("DenoiseGI", DXGI_FORMAT_R8G8B8A8_UNORM, LoadOp::Load, StoreOp::Store);

		_cpBuilder.ResolveAndCompile(a_pPSOManager);

		_node.executeFunc = [_spPassData](GraphicsEngine* a_pGE, RenderContext* a_pCtx, uint8_t a_passIndex)
		{
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
				_pCmd,
				0,
				_op
			);

			a_pCtx->BindHeap();
			std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> _cpuVec = {
				_spPassData->pRG->GetSRVCPU("RayGI"),
				_spPassData->pRG->GetSRVCPU("GBufferVelocity"),
				_spPassData->pRG->GetSRVCPU("DenoiseGI"),
				_spPassData->pRG->GetSRVCPU("Depth"),
				_spPassData->pRG->GetSRVCPU("GBufferNormal"),
				_spPassData->pRG->GetSRVCPU("PrevDepth"),
				_spPassData->pRG->GetSRVCPU("PrevNormal")
			};
			a_pCtx->ComputeBindSRV(1, _cpuVec);

			a_pCtx->BindUAV(2, _spPassData->pRG->GetUAVCPU("DenoiseGI"));
			// 実行
			UINT _countX = _winOp.windowWidth / 8;
			UINT _countY = _winOp.windowHegiht / 8;
			a_pCtx->Dispatch(_countX, _countY, 1);
		};

		a_rg.AddPassNode(a_phase, _node);
	}
}
