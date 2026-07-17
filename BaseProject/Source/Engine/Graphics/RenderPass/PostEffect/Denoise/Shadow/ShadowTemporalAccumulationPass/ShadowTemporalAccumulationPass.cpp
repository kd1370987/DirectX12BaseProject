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
		// 後続のパス（ライティング等）が常に同じ固定名で最新の影を参照できるようにする最終出力先
		const std::string _finalDst = "AffterDLShadowTempAccumu";

		// ======================================================================
		// 偶数フレーム用(A->B)と奇数フレーム用(B->A)の2つのパスセットを登録する
		// ======================================================================
		for (int i = 0; i < 2; ++i)
		{
			const bool _isEven = (i == 0); // true: 偶数フレーム用, false: 奇数フレーム用
			const ERGFrameParity _parity = _isEven ? ERGFrameParity::Even : ERGFrameParity::Odd;

			// フレームの偶奇に合わせて読み書きするヒストリーバッファの名前を固定化
			const std::string _readHistory  = _isEven ? "ShadowHistory_A" : "ShadowHistory_B";
			const std::string _writeHistory = _isEven ? "ShadowHistory_B" : "ShadowHistory_A";

			// ------------------------------------------------------------------
			// 1. TemporalAccumulation パス（コンピュートによるノイズ除去と履歴合成）
			// ------------------------------------------------------------------
			RenderPassNode _node = {};
			_node.name = _isEven ? "ShadowTADenoisePass_Even" : "ShadowTADenoisePass_Odd";
			_node.phase = a_phase;
			RGComputePassBuilder _cpBuilder(&_node);

			// シェーダーのコンパイルとルートシグネチャの取得
			uint8_t _csIndex = RenderPassNode::kInvalidPSOIndex;
			auto* _pBlob = _cpBuilder.SetShader("Asset/Shader/Compute/Denoise/Shadow/ShadowTemporalAccumullationShader.cso", "ShadowTemporalAccumullationShader", _csIndex);
			_cpBuilder.SetRootSignature(a_pPSOManager, _pBlob);
			_cpBuilder.SetHeapMode(ERGHeapMode::Default);
			_cpBuilder.SetFrameParity(_parity);

			// 依存関係とバインドの宣言（宣言順 = t0～t6）
			_cpBuilder.SrvTable(1)
				.Add("RayShadow")
				.Add("GBufferVelocity")
				.Add(_readHistory)
				.Add("Depth")
				.Add("GBufferNormal")
				.Add("PrevDepth")
				.Add("PrevNormal");

			_cpBuilder.BindUAV(2, _writeHistory, DXGI_FORMAT_R8G8B8A8_UNORM, LoadOp::Clear, StoreOp::Store);

			_cpBuilder.ResolveAndCompile(a_pPSOManager);

			// 実行関数 : オプション由来の定数バッファとディスパッチのみ
			_node.executeFunc = [](GraphicsEngine* a_pGE, RenderContext* a_pCtx, const RGPassResources& a_res)
				{
					const auto& _winOp = Option::OptionManager::GetInstance().GetWindowOption();
					auto* _pCmd = a_pCtx->GetCurrentCmdList();

					// 定数バッファのバインド
					struct GITAOp { float phiDepth; float phiNormal; float blendRate; };
					const auto& _giOp = Option::OptionManager::GetInstance().GetGIOption();
					GITAOp _op = {};
					_op.phiDepth  = _giOp.TAphiDepth;
					_op.phiNormal = _giOp.TAphiNormal;
					_op.blendRate = _giOp.TAblendRate;
					a_pCtx->BindCB()->BindAndAttachDataComputeRootCBV(_pCmd, 0, _op);

					// コンピュートシェーダーのディスパッチ
					a_pCtx->Dispatch(_winOp.windowWidth / 8, _winOp.windowHegiht / 8, 1);
				};
			a_pRegistry->RegisterPass(_node);

			// ------------------------------------------------------------------
			// 2. コピーパス（書き込んだ最新の履歴を、後続パス用の共通名リソースへコピー）
			// 宣言だけで完結するので実行関数は不要
			// ------------------------------------------------------------------
			RenderPassNode _copyNode = {};
			_copyNode.name = _isEven ? "ShadowHistoryCopyPass_Even" : "ShadowHistoryCopyPass_Odd";
			_copyNode.phase = a_phase;
			RGGlobalsPassBuilder _copyBuilder(&_copyNode);

			_copyBuilder.SetFrameParity(_parity);
			_copyBuilder.Copy(_writeHistory, _finalDst, DXGI_FORMAT_R8G8B8A8_UNORM);

			a_pRegistry->RegisterPass(_copyNode);
		}
	}
}
