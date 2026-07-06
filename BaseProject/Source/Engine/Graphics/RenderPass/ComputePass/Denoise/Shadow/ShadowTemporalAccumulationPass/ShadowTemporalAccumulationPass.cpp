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
		// ======================================================================
		// ランタイムデータ（コンピュートパスで共通使用するもの）
		// ======================================================================
		struct RuntimeData
		{
			ID3D12RootSignature* pRootSig;
			uint8_t csIndex;
			D3D12::PipelineStateManager* pPSOManager;
		};
		auto _spPassData = std::make_shared<RuntimeData>();
		_spPassData->pPSOManager = a_pPSOManager;

		// 後続のパス（ライティング等）が常に同じ固定名で最新の影を参照できるようにする最終出力先
		std::string _finalDst = "AffterDLShadowTempAccumu";

		// ======================================================================
		// 偶数フレーム用(A->B)と奇数フレーム用(B->A)の2つのパスセットを登録する
		// ======================================================================
		for (int i = 0; i < 2; ++i)
		{
			bool _isEven = (i == 0); // true: 偶数フレーム用, false: 奇数フレーム用

			// フレームの偶奇に合わせて読み書きするヒストリーバッファの名前を固定化
			std::string _readHistory = _isEven ? "ShadowHistory_A" : "ShadowHistory_B";
			std::string _writeHistory = _isEven ? "ShadowHistory_B" : "ShadowHistory_A";

			std::string _passName = _isEven ? "ShadowTADenoisePass_Even" : "ShadowTADenoisePass_Odd";
			std::string _copyPassName = _isEven ? "ShadowHistoryCopyPass_Even" : "ShadowHistoryCopyPass_Odd";

			// ------------------------------------------------------------------
			// 1. TemporalAccumulation パス（コンピュートによるノイズ除去と履歴合成）
			// ------------------------------------------------------------------
			RenderPassNode _node = {};
			_node.name = _passName;
			_node.phase = a_phase;
			RGComputePassBuilder _cpBuilder(&_node);

			// 依存関係の構築（Setupフェーズ）
			_cpBuilder.ReadSRV("RayShadow");
			_cpBuilder.ReadSRV("GBufferVelocity");
			_cpBuilder.ReadSRV(_readHistory);
			_cpBuilder.ReadSRV("Depth");
			_cpBuilder.ReadSRV("GBufferNormal");
			_cpBuilder.ReadSRV("PrevDepth");
			_cpBuilder.ReadSRV("PrevNormal");

			_cpBuilder.WriteUAV(_writeHistory, DXGI_FORMAT_R8G8B8A8_UNORM, LoadOp::Clear, StoreOp::Store);

			// シェーダーのコンパイルとルートシグネチャの取得
			auto* _pBlob = _cpBuilder.SetShader("Asset/Shader/Compute/Denoise/Shadow/ShadowTemporalAccumullationShader.cso", "ShadowTemporalAccumullationShader", _spPassData->csIndex);
			_spPassData->pRootSig = _cpBuilder.SetRootSignature(a_pPSOManager, _pBlob);

			_cpBuilder.ResolveAndCompile(a_pPSOManager);

			// 実行関数
			_node.executeFunc = [_spPassData, _readHistory, _writeHistory, _isEven](GraphicsEngine* a_pGE, RenderContext* a_pCtx, uint8_t a_passIndex)
				{
					// 自分の担当フレーム（偶数/奇数）でなければスキップ
					UINT64 _currentFrame = a_pGE->RefRenderGraph()->GetTemporalIndex();
					bool _isCurrentEven = (_currentFrame % 2 == 0);
					if (_isEven != _isCurrentEven) return;

					auto* _pRG = a_pGE->RefRenderGraph();
					auto* _pCmd = a_pCtx->GetCurrentCmdList();
					const auto& _winOp = Option::OptionManager::GetInstance().GetWindowOption();

					// パイプラインステートの設定
					_pCmd->SetComputeRootSignature(_spPassData->pRootSig);
					_pCmd->SetPipelineState(_spPassData->pPSOManager->GetPSO(_spPassData->csIndex));

					// 定数バッファのバインド
					struct GITAOp { float phiDepth; float phiNormal; float blendRate; };
					GITAOp _op = {};
					const auto& _giOp = Option::OptionManager::GetInstance().GetGIOption();
					_op.phiDepth = _giOp.TAphiDepth;
					_op.phiNormal = _giOp.TAphiNormal;
					_op.blendRate = _giOp.TAblendRate;
					a_pCtx->BindCB()->BindAndAttachDataComputeRootCBV(_pCmd, 0, _op);

					// SRVのバインド（レンダーグラフから安全にハンドルを取得）
					a_pCtx->BindHeap();
					std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> _cpuVec = {
						_pRG->GetPassSRV(a_passIndex, "RayShadow"),
						_pRG->GetPassSRV(a_passIndex, "GBufferVelocity"),
						_pRG->GetPassSRV(a_passIndex, _readHistory),
						_pRG->GetPassSRV(a_passIndex, "Depth"),
						_pRG->GetPassSRV(a_passIndex, "GBufferNormal"),
						_pRG->GetPassSRV(a_passIndex, "PrevDepth"),
						_pRG->GetPassSRV(a_passIndex, "PrevNormal")
					};
					a_pCtx->ComputeBindSRV(1, _cpuVec);

					// UAVのバインド
					a_pCtx->BindUAV(2, _pRG->GetPassUAV(a_passIndex, _writeHistory));

					// コンピュートシェーダーのディスパッチ
					UINT _countX = _winOp.windowWidth / 8;
					UINT _countY = _winOp.windowHegiht / 8;
					a_pCtx->Dispatch(_countX, _countY, 1);
				};
			a_pRegistry->RegisterPass(_node);

			// ------------------------------------------------------------------
			// 2. コピーパス（書き込んだ最新の履歴を、後続パス用の共通名リソースへコピー）
			// ------------------------------------------------------------------
			RenderPassNode _copyNode = {};
			_copyNode.name = _copyPassName;
			_copyNode.phase = a_phase;
			RGGlobalsPassBuilder _copyBuilder(&_copyNode);

			// 依存関係の構築
			_copyBuilder.CopySrc(_writeHistory);
			_copyBuilder.CopyDst(_finalDst, DXGI_FORMAT_R8G8B8A8_UNORM);

			// 実行関数
			// ※ コピー処理には _spPassData は不要なのでキャプチャから外しています
			_copyNode.executeFunc = [_writeHistory, _finalDst, _isEven](GraphicsEngine* a_pGE, RenderContext* a_pCtx, uint8_t a_passIndex)
				{
					// 同様に担当フレーム以外はスキップ
					UINT64 _currentFrame = a_pGE->RefRenderGraph()->GetTemporalIndex();
					bool _isCurrentEven = (_currentFrame % 2 == 0);
					if (_isEven != _isCurrentEven) return;

					auto* _pRG = a_pGE->RefRenderGraph();

					// レンダーグラフから物理リソース（D3D12::GPUResource*）を直接取得
					auto* _pSrcResource = _pRG->GetPassResource(a_passIndex, _writeHistory);
					auto* _pDstResource = _pRG->GetPassResource(a_passIndex, _finalDst);

					// リソース間コピーを実行（RenderContext側をGPUResource*対応にする必要があります）
					a_pCtx->ResourceCopy(_pSrcResource->GetResource(), _pDstResource->GetResource());
				};
			a_pRegistry->RegisterPass(_copyNode);
		}
	}
}