#include "TAAPass.h"

#include "Engine/Graphics/RenderPassRegistry/RenderPassRegistry.h"
#include "Engine/Graphics/RenderGraph/RenderGraph.h"
#include "Engine/Graphics/RenderGraph/RGPassBuilder/RGPassBuilder.h"
#include "Engine/Graphics/GraphicEngine.h"

#include "Engine/Graphics/RenderContext/RenderContext.h"
#include "Engine/D3D12/PipelineStateManager/PipelineStateManager.h"

#include "../../../../../Option/OptionManager.h"

namespace Engine::Graphics
{
	void Engine::Graphics::AddTAAPass(
		D3D12::PipelineStateManager* a_pPSOManager, 
		RenderPassRegistry* a_pRegistry, 
		const EDrawPhase& a_phase
	)
	{
		// ======================================================================
		// ランタイムデータ取得
		// ======================================================================
		struct RuntimeData
		{
			ID3D12RootSignature* pRootSig;
			uint8_t csIndex;
			D3D12::PipelineStateManager* pPSOManager;
		};
		auto _spPassData = std::make_shared<RuntimeData>();
		_spPassData->pPSOManager = a_pPSOManager;

		// 後続のパス（ポストプロセス等）が常に同じ固定名で最新のTAA結果を参照できるようにする宛先
		std::string _finalDst = "AffterTAAColor";

		// ======================================================================
		// 偶数フレーム用と奇数フレーム用の2つのパスセットを登録する
		// ======================================================================
		for (int i = 0; i < 2; ++i)
		{
			bool _isEven = (i == 0); // true: 偶数フレーム用, false: 奇数フレーム用

			// フレームの偶奇に合わせて読み書きするヒストリーバッファの名前を固定化
			std::string _readHistory = _isEven ? "TAAHistory_A" : "TAAHistory_B";
			std::string _writeHistory = _isEven ? "TAAHistory_B" : "TAAHistory_A";

			std::string _passName = _isEven ? "TAAPass_Even" : "TAAPass_Odd";
			std::string _copyPassName = _isEven ? "TAACopyPass_Even" : "TAACopyPass_Odd";

			// ------------------------------------------------------------------
			// TAA パス（コンピュートによる合成）
			// ------------------------------------------------------------------
			RenderPassNode _node = {};
			_node.name = _passName;
			_node.phase = a_phase;
			RGComputePassBuilder _cpBuilder(&_node);

			// 依存関係構築 (Setupフェーズ)
			_cpBuilder.ReadSRV("AffterLighting");
			_cpBuilder.ReadSRV(_readHistory);      // TAA履歴を読み込む
			_cpBuilder.ReadSRV("GBufferVelocity");
			_cpBuilder.ReadSRV("Depth");
			_cpBuilder.ReadSRV("GBufferNormal");

			// 結果を書き込むUAV
			_cpBuilder.WriteUAV(_writeHistory, DXGI_FORMAT_R8G8B8A8_UNORM, LoadOp::Clear, StoreOp::Store);

			// シェーダーセット
			auto* _pBlob = _cpBuilder.SetShader("Asset/Shader/Compute/AntiAliasing/TAA/TAA.cso", "TAAShader", _spPassData->csIndex);
			// ルートシグネチャセット
			_spPassData->pRootSig = _cpBuilder.SetRootSignature(a_pPSOManager, _pBlob);

			// PSO作成
			_cpBuilder.ResolveAndCompile(a_pPSOManager);

			// 実行関数
			_node.executeFunc = [_spPassData, _readHistory, _writeHistory, _isEven](GraphicsEngine* a_pGE, RenderContext* a_pCtx, uint8_t a_passIndex)
				{
					// 自分の担当フレーム（偶数/奇数）でなければスキップ
					UINT64 _currentFrame = a_pGE->RefRenderGraph()->GetTemporalIndex();
					bool _isCurrentEven = (_currentFrame % 2 == 0);
					if (_isEven != _isCurrentEven) return;

					// オプション取得
					const auto& _winOp = Option::OptionManager::GetInstance().GetWindowOption();

					auto* _pRG = a_pGE->RefRenderGraph();
					auto* _pCmd = a_pCtx->GetCurrentCmdList();

					// ヒープとルートシグネチャ、PSOをセット
					auto* _pPso = _spPassData->pPSOManager->GetPSO(_spPassData->csIndex);
					a_pCtx->BindHeap();
					_pCmd->SetComputeRootSignature(_spPassData->pRootSig);
					a_pCtx->SetComputePSO(_pPso);

					// SRVのバインド（レンダーグラフからパスインデックスを使って安全に取得）
					std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> _gpuVec = {
						_pRG->GetPassSRV(a_passIndex, "AffterLighting"),
						_pRG->GetPassSRV(a_passIndex, _readHistory),
						_pRG->GetPassSRV(a_passIndex, "GBufferVelocity"),
						_pRG->GetPassSRV(a_passIndex, "Depth"),
						_pRG->GetPassSRV(a_passIndex, "GBufferNormal")
					};
					a_pCtx->ComputeBindSRV(0, _gpuVec);

					// UAVのバインド
					a_pCtx->BindUAV(1, _pRG->GetPassUAV(a_passIndex, _writeHistory));

					// 実行
					UINT _countX = _winOp.windowWidth / 8;
					UINT _countY = _winOp.windowHegiht / 8;
					a_pCtx->Dispatch(_countX, _countY, 1);
				};
			a_pRegistry->RegisterPass(_node);

			// ------------------------------------------------------------------
			// コピーパス（後続のパスが固定名で参照できるようにする）
			// ------------------------------------------------------------------
			RenderPassNode _copyNode = {};
			_copyNode.name = _copyPassName;
			_copyNode.phase = a_phase;
			RGGlobalsPassBuilder _copyBuilder(&_copyNode);

			// コピー元の最新履歴と、コピー先の最終出力
			_copyBuilder.CopySrc(_writeHistory);
			_copyBuilder.CopyDst(_finalDst, DXGI_FORMAT_R8G8B8A8_UNORM);

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

					// リソース間コピーを実行
					a_pCtx->ResourceCopy(_pSrcResource->GetResource(), _pDstResource->GetResource());
				};
			a_pRegistry->RegisterPass(_copyNode);
		}
	}
}