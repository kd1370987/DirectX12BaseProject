#include "GITemporalAccumulationPass.h"

#include "Engine/Graphics/RenderPassRegistry/RenderPassRegistry.h"
#include "Engine/Graphics/RenderGraph/RenderGraph.h"
#include "Engine/Graphics/RenderGraph/RGPassBuilder/RGPassBuilder.h"
#include "Engine/Graphics/GraphicEngine.h"

#include "Engine/Graphics/RenderContext/RenderContext.h"
#include "Engine/D3D12/PipelineStateManager/PipelineStateManager.h"

#include "../../../../../../Option/OptionManager.h"

namespace Engine::Graphics
{
	void AddGITemporalAccumulationPass(D3D12::PipelineStateManager* a_pPSOManager, RenderPassRegistry* a_pRegistry, const EDrawPhase& a_phase)
	{
		// ======================================================================
		// ランタイムデータ作成
		// ======================================================================
		struct RuntimeData
		{
			ID3D12RootSignature* pRootSig;
			uint8_t csIndex;
			D3D12::PipelineStateManager* pPSOManager;
		};
		auto _spPassData = std::make_shared<RuntimeData>();
		_spPassData->pPSOManager = a_pPSOManager;

		// 後続のパス（GISpatialDenoisePass）が常に同じ固定名で参照できるようにする宛先
		std::string _finalDst = "DenoiseGI";

		// ======================================================================
		// 偶数フレーム用と奇数フレーム用の2つのパスセットを登録する
		// ======================================================================
		for (int i = 0; i < 2; ++i)
		{
			bool _isEven = (i == 0); // true: 偶数フレーム用, false: 奇数フレーム用

			// フレームの偶奇に合わせて読み書きするヒストリーバッファの名前を固定化
			std::string _readHistory = _isEven ? "DenoiseGI_History_A" : "DenoiseGI_History_B";
			std::string _writeHistory = _isEven ? "DenoiseGI_History_B" : "DenoiseGI_History_A";

			std::string _passName = _isEven ? "TemporalAccumulationPass_Even" : "TemporalAccumulationPass_Odd";
			std::string _copyPassName = _isEven ? "TemporalAccumulationCopyPass_Even" : "TemporalAccumulationCopyPass_Odd";

			// ------------------------------------------------------------------
			// 1. TemporalAccumulation パス
			// ------------------------------------------------------------------
			RenderPassNode _node = {};
			_node.name = _passName;
			_node.phase = a_phase;
			RGComputePassBuilder _cpBuilder(&_node);

			// シェーダーとルートシグネチャの設定
			auto* _pBlob = _cpBuilder.SetShader("Asset/Shader/Compute/TemporalAccumulationShader/TemporalAccumulationShader.cso", "TemporalAccumulationPass", _spPassData->csIndex);
			_spPassData->pRootSig = _cpBuilder.SetRootSignature(a_pPSOManager, _pBlob);

			// 依存関係構築（Setupフェーズ）
			_cpBuilder.ReadSRV("RayGI");
			_cpBuilder.ReadSRV("GBufferVelocity");
			_cpBuilder.ReadSRV(_readHistory); // 旧 ReadHistorySRV に代わる処理
			_cpBuilder.ReadSRV("Depth");
			_cpBuilder.ReadSRV("GBufferNormal");
			_cpBuilder.ReadSRV("PrevDepth");
			_cpBuilder.ReadSRV("PrevNormal");

			// UAVへの書き込み
			_cpBuilder.WriteUAV(_writeHistory, DXGI_FORMAT_R8G8B8A8_UNORM, LoadOp::Clear, StoreOp::Store);

			_cpBuilder.ResolveAndCompile(a_pPSOManager);

			// 実行関数
			_node.executeFunc = [_spPassData, _readHistory, _writeHistory, _isEven](GraphicsEngine* a_pGE, RenderContext* a_pCtx, uint8_t a_passIndex)
				{
					// 担当フレームチェック
					UINT64 _currentFrame = a_pGE->RefRenderGraph()->GetTemporalIndex();
					bool _isCurrentEven = (_currentFrame % 2 == 0);
					if (_isEven != _isCurrentEven) return;

					// オプション取得 (GetInstance()の重複を修正)
					const auto& _winOp = Option::OptionManager::GetInstance().GetWindowOption();

					auto* _pRG = a_pGE->RefRenderGraph();
					auto* _pCmd = a_pCtx->GetCurrentCmdList();

					// パイプラインステートの設定
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
					a_pCtx->BindCB()->BindAndAttachDataComputeRootCBV(_pCmd, 0, _op);

					// SRVバインド（レンダーグラフからパスインデックスを使って安全に取得）
					a_pCtx->BindHeap();
					std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> _cpuVec = {
						_pRG->GetPassSRV(a_passIndex, "RayGI"),
						_pRG->GetPassSRV(a_passIndex, "GBufferVelocity"),
						_pRG->GetPassSRV(a_passIndex, _readHistory),
						_pRG->GetPassSRV(a_passIndex, "Depth"),
						_pRG->GetPassSRV(a_passIndex, "GBufferNormal"),
						_pRG->GetPassSRV(a_passIndex, "PrevDepth"),
						_pRG->GetPassSRV(a_passIndex, "PrevNormal")
					};
					a_pCtx->ComputeBindSRV(1, _cpuVec);

					// UAVバインド
					a_pCtx->BindUAV(2, _pRG->GetPassUAV(a_passIndex, _writeHistory));

					// 実行
					UINT _countX = _winOp.windowWidth / 8;
					UINT _countY = _winOp.windowHegiht / 8; // ※windowHegihtは元のスペル
					a_pCtx->Dispatch(_countX, _countY, 1);
				};

			a_pRegistry->RegisterPass(_node);

			// ------------------------------------------------------------------
			// 2. コピーパス（SpatialDenoise等へ安定したリソース名を提供するため）
			// ------------------------------------------------------------------
			RenderPassNode _copyNode = {};
			_copyNode.name = _copyPassName;
			_copyNode.phase = a_phase;
			RGGlobalsPassBuilder _copyBuilder(&_copyNode);

			// 依存関係構築
			_copyBuilder.CopySrc(_writeHistory);
			_copyBuilder.CopyDst(_finalDst, DXGI_FORMAT_R8G8B8A8_UNORM);

			_copyNode.executeFunc = [_writeHistory, _finalDst, _isEven](GraphicsEngine* a_pGE, RenderContext* a_pCtx, uint8_t a_passIndex)
				{
					// 同様に担当フレーム以外はスキップ
					UINT64 _currentFrame = a_pGE->RefRenderGraph()->GetTemporalIndex();
					bool _isCurrentEven = (_currentFrame % 2 == 0);
					if (_isEven != _isCurrentEven) return;

					auto* _pRG = a_pGE->RefRenderGraph();

					// 物理リソースを直接取得
					auto* _pSrcResource = _pRG->GetPassResource(a_passIndex, _writeHistory);
					auto* _pDstResource = _pRG->GetPassResource(a_passIndex, _finalDst);

					// コピー実行
					a_pCtx->ResourceCopy(_pSrcResource->GetResource(), _pDstResource->GetResource());
				};

			a_pRegistry->RegisterPass(_copyNode);
		}
	}
}
