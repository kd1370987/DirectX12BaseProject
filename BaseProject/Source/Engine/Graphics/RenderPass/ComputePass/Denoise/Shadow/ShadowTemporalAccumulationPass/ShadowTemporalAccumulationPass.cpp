#include "ShadowTemporalAccumulationPass.h"
#include "Engine/Graphics/RenderGraph/RenderGraph.h"
#include "Engine/Graphics/RenderGraph/RGPassBuilder/RGPassBuilder.h"
#include "Engine/Graphics/GraphicEngine.h"
#include "Engine/Graphics/RenderContext/RenderContext.h"
#include "Engine/D3D12/PipelineStateManager/PipelineStateManager.h"
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

		// 後続のパス（ライティング等）が常に同じ固定名で最新の影を参照できるようにする宛先
		std::string _finalDst = "AffterDLShadowTempAccumu";

		// ----------------------------------------------------------------------
		// 解決策2: 偶数フレーム用(Even)と奇数フレーム用(Odd)の2つのパスを登録する
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
			// 1. Temporal Accumulation パスの構築
			// ==========================================
			RenderPassNode _node = {};
			_node.name = _passName;
			RGComputePassBuilder _cpBuilder(&_node, &a_rg);

			_spPassData->pRootSig = _cpBuilder.SetRootSignature(a_pPSOManager, "Asset/Shader/Compute/Denoise/Shadow/ShadowTemporalAccumullationShader.cso");
			_cpBuilder.SetShader("Asset/Shader/Compute/Denoise/Shadow/ShadowTemporalAccumullationShader.cso", "ShadowTemporalAccumullationShader", _spPassData->csIndex);

			// 依存関係構築 (固定された文字列を使う)
			_cpBuilder.Read("RayShadow", AccessType::SRV, LoadOp::Load, StoreOp::Store);
			_cpBuilder.Read("GBufferVelocity", AccessType::SRV, LoadOp::Load, StoreOp::Store);
			_cpBuilder.Read(_readHistory, AccessType::SRV, LoadOp::Load, StoreOp::Store);
			_cpBuilder.Read("Depth", AccessType::SRV, LoadOp::Load, StoreOp::Store);
			_cpBuilder.Read("GBufferNormal", AccessType::SRV, LoadOp::Load, StoreOp::Store);
			_cpBuilder.Read("PrevDepth", AccessType::SRV, LoadOp::Load, StoreOp::Store);
			_cpBuilder.Read("PrevNormal", AccessType::SRV, LoadOp::Load, StoreOp::Store);

			_cpBuilder.Write(_writeHistory, AccessType::UAV, LoadOp::Clear, StoreOp::Store);

			_cpBuilder.ResolveAndCompile(a_pPSOManager);

			// 実行関数
			_node.executeFunc = [_spPassData, _readHistory, _writeHistory, isEven, _passName](GraphicsEngine* a_pGE, RenderContext* a_pCtx, uint8_t a_passIndex)
				{
					// ★ここがミソ：実行時に自分の担当フレームかチェックし、違えば何もしない
					UINT64 _currentFrame = _spPassData->pRG->GetTemporalIndex();
					bool _isCurrentEven = (_currentFrame % 2 == 0);
					if (isEven != _isCurrentEven) return;

					Editor::MainEditor::Instance().StartWatch(_passName);

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
						_spPassData->pRG->GetSRVCPU("RayShadow"),
						_spPassData->pRG->GetSRVCPU("GBufferVelocity"),
						_spPassData->pRG->GetSRVCPU(_readHistory),
						_spPassData->pRG->GetSRVCPU("Depth"),
						_spPassData->pRG->GetSRVCPU("GBufferNormal"),
						_spPassData->pRG->GetSRVCPU("PrevDepth"),
						_spPassData->pRG->GetSRVCPU("PrevNormal")
					};
					a_pCtx->ComputeBindSRV(1, _cpuVec);
					a_pCtx->BindUAV(2, _spPassData->pRG->GetUAVCPU(_writeHistory));

					// 実行
					UINT _countX = _winOp.windowWidth / 8;
					UINT _countY = _winOp.windowHegiht / 8;
					a_pCtx->Dispatch(_countX, _countY, 1);

					Editor::MainEditor::Instance().EndWatch(_passName);
				};
			a_rg.AddPassNode(a_phase, _node);

			// ==========================================
			// 2. コピーパスの構築 (最新結果を固定名へ)
			// ==========================================
			RenderPassNode _copyNode = {};
			_copyNode.name = _copyPassName;
			RGComputePassBuilder _copyBuilder(&_copyNode, &a_rg);
			_copyBuilder.Read(_writeHistory, AccessType::CopySrc, LoadOp::Load, StoreOp::Store);
			_copyBuilder.Write(_finalDst, AccessType::CopyDst, LoadOp::Clear, StoreOp::Store);

			_copyNode.executeFunc = [_spPassData, _writeHistory, _finalDst, isEven, _copyPassName](GraphicsEngine* a_pGE, RenderContext* a_pCtx, uint8_t a_passIndex)
				{
					// ★同様に担当フレーム以外はスキップ
					UINT64 _currentFrame = _spPassData->pRG->GetTemporalIndex();
					bool _isCurrentEven = (_currentFrame % 2 == 0);
					if (isEven != _isCurrentEven) return;

					Editor::MainEditor::Instance().StartWatch(_copyPassName);

					const auto& _srcTAATexHandle = _spPassData->pRG->GetTexHandle(_writeHistory);
					const auto& _dstTAATexHandle = _spPassData->pRG->GetTexHandle(_finalDst);
					a_pCtx->TexCopy(_srcTAATexHandle, _dstTAATexHandle);

					Editor::MainEditor::Instance().EndWatch(_copyPassName);
				};
			a_rg.AddPassNode(a_phase, _copyNode);
		}
	}
}