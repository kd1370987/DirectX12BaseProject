#include "ShadowTemporalAccumulationPass.h"
#include "Engine/Graphics/RenderPassRegistry/RenderPassRegistry.h"
#include "Engine/Graphics/RenderGraph/RenderGraph.h"
#include "Engine/Graphics/RenderGraph/RGPassBuilder/RGPassBuilder.h"
#include "Engine/Graphics/GraphicEngine.h"
#include "Engine/Graphics/RenderContext/RenderContext.h"
#include "Engine/D3D12/PipelineStateManager/PipelineStateManager.h"
#include "Engine/Option/OptionManager.h"

namespace Engine::Graphics
{

	void AddShadowTemporalAccumulationPass(D3D12::PipelineStateManager* a_pPSOManager, RenderPassRegistry* a_pRegistry, const EDrawPhase& a_phase)
	{
		// ランタイムデータ
		struct RuntimeData
		{
			ID3D12RootSignature* pRootSig;
			uint8_t csIndex;
			D3D12::PipelineStateManager* pPSOManager;
			
		};
		auto _spPassData = std::make_shared<RuntimeData>();
		_spPassData->pPSOManager = a_pPSOManager;
		

		// 後続のパス（ライティング等）が常に同じ固定名で最新の影を参照できるようにする宛先
		std::string _finalDst = "AffterDLShadowTempAccumu";

		// ----------------------------------------------------------------------
		// 偶数フレーム用と奇数フレーム用の2つのパスを登録する
		// ----------------------------------------------------------------------
		for (int i = 0; i < 2; ++i)
		{
			bool isEven = (i == 0); // true: 偶数フレーム用パス, false: 奇数フレーム用パス

			// 構築時に名前を完全に固定化する
			std::string _readHistory = isEven ? "ShadowHistory_A" : "ShadowHistory_B";
			std::string _writeHistory = isEven ? "ShadowHistory_B" : "ShadowHistory_A";
			std::string _passName = isEven ? "ShadowTADenoisePass_Even" : "ShadowTADenoisePass_Odd";
			std::string _copyPassName = isEven ? "ShadowHistoryCopyPass_Even" : "ShadowHistoryCopyPass_Odd";

			// ==========================================
			// TemporalAccumulation パスの構築
			// ==========================================
			RenderPassNode _node = {};
			_node.name = _passName;
			_node.phase = a_phase;
			RGComputePassBuilder _cpBuilder(&_node);

			// 依存関係構築 
			_cpBuilder.ReadSRV("RayShadow");
			_cpBuilder.ReadSRV("GBufferVelocity");
			_cpBuilder.ReadSRV(_readHistory);
			_cpBuilder.ReadSRV("Depth");
			_cpBuilder.ReadSRV("GBufferNormal");
			_cpBuilder.ReadSRV("PrevDepth");
			_cpBuilder.ReadSRV("PrevNormal");

			_cpBuilder.WriteUAV(_writeHistory, DXGI_FORMAT_R8G8B8A8_UNORM, LoadOp::Clear, StoreOp::Store);

			auto* _pBlob = _cpBuilder.SetShader("Asset/Shader/Compute/Denoise/Shadow/ShadowTemporalAccumullationShader.cso", "ShadowTemporalAccumullationShader", _spPassData->csIndex);
			_spPassData->pRootSig = _cpBuilder.SetRootSignature(a_pPSOManager, _pBlob);

			_cpBuilder.ResolveAndCompile(a_pPSOManager);

			// 実行関数
			_node.executeFunc = [_spPassData, _readHistory, _writeHistory, isEven, _passName](GraphicsEngine* a_pGE, RenderContext* a_pCtx, uint8_t a_passIndex)
				{
					// 実行時に自分の担当フレームかチェックし、違えば何もしない
					UINT64 _currentFrame = a_pGE->RefRenderGraph()->GetTemporalIndex();
					bool _isCurrentEven = (_currentFrame % 2 == 0);
					if (isEven != _isCurrentEven) return;

					const auto& _winOp = Option::OptionManager::GetInstance().GetInstance().GetWindowOption();
					auto* _pCmd = a_pCtx->GetCurrentCmdList();
					_pCmd->SetComputeRootSignature(_spPassData->pRootSig);
					_pCmd->SetPipelineState(_spPassData->pPSOManager->GetPSO(_spPassData->csIndex));

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
					a_pCtx->BindCB()->BindAndAttachDataComputeRootCBV(_pCmd, 0, _op);

					// バインド
					a_pCtx->BindHeap();
					std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> _cpuVec = {
						a_pGE->RefRenderGraph()->GetSRVCPU("RayShadow"),
						a_pGE->RefRenderGraph()->GetSRVCPU("GBufferVelocity"),
						a_pGE->RefRenderGraph()->GetSRVCPU(_readHistory),
						a_pGE->RefRenderGraph()->GetSRVCPU("Depth"),
						a_pGE->RefRenderGraph()->GetSRVCPU("GBufferNormal"),
						a_pGE->RefRenderGraph()->GetSRVCPU("PrevDepth"),
						a_pGE->RefRenderGraph()->GetSRVCPU("PrevNormal")
					};
					a_pCtx->ComputeBindSRV(1, _cpuVec);
					a_pCtx->BindUAV(2, a_pGE->RefRenderGraph()->GetUAVCPU(_writeHistory));

					// 実行
					UINT _countX = _winOp.windowWidth / 8;
					UINT _countY = _winOp.windowHegiht / 8;
					a_pCtx->Dispatch(_countX, _countY, 1);
				};
			_node.phase = a_phase;
			a_pRegistry->RegisterPass(_node);

			// ==========================================
			// コピーパスの構築 
			// ==========================================
			RenderPassNode _copyNode = {};
			_copyNode.name = _copyPassName;
			_copyNode.phase = a_phase;
			RGGlobalsPassBuilder _copyBuilder(&_copyNode);
			_copyBuilder.CopySrc(_writeHistory);
			_copyBuilder.CopyDst(_finalDst, DXGI_FORMAT_R8G8B8A8_UNORM);

			_copyNode.executeFunc = [_spPassData, _writeHistory, _finalDst, isEven, _copyPassName](GraphicsEngine* a_pGE, RenderContext* a_pCtx, uint8_t a_passIndex)
				{
					// 同様に担当フレーム以外はスキップ
					UINT64 _currentFrame = a_pGE->RefRenderGraph()->GetTemporalIndex();
					bool _isCurrentEven = (_currentFrame % 2 == 0);
					if (isEven != _isCurrentEven) return;

					const auto& _srcTAATexHandle = a_pGE->RefRenderGraph()->GetTexHandle(_writeHistory);
					const auto& _dstTAATexHandle = a_pGE->RefRenderGraph()->GetTexHandle(_finalDst);
					a_pCtx->TexCopy(_srcTAATexHandle, _dstTAATexHandle);
				};
			a_pRegistry->RegisterPass(_copyNode);
		}
	}
}