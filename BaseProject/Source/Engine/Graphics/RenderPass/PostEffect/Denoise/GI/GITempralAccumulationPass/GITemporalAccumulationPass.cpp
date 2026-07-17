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
		// 後続のパス（GISpatialDenoisePass）が常に同じ固定名で参照できるようにする宛先
		const std::string _finalDst = "DenoiseGI";

		// ======================================================================
		// 偶数フレーム用と奇数フレーム用の2つのパスセットを登録する
		// ======================================================================
		for (int i = 0; i < 2; ++i)
		{
			const bool _isEven = (i == 0); // true: 偶数フレーム用, false: 奇数フレーム用
			const ERGFrameParity _parity = _isEven ? ERGFrameParity::Even : ERGFrameParity::Odd;

			// フレームの偶奇に合わせて読み書きするヒストリーバッファの名前を固定化
			const std::string _readHistory  = _isEven ? "DenoiseGI_History_A" : "DenoiseGI_History_B";
			const std::string _writeHistory = _isEven ? "DenoiseGI_History_B" : "DenoiseGI_History_A";

			// ------------------------------------------------------------------
			// 1. TemporalAccumulation パス
			// ------------------------------------------------------------------
			RenderPassNode _node = {};
			_node.name = _isEven ? "TemporalAccumulationPass_Even" : "TemporalAccumulationPass_Odd";
			_node.phase = a_phase;
			RGComputePassBuilder _cpBuilder(&_node);

			// シェーダーとルートシグネチャの設定
			uint8_t _csIndex = RenderPassNode::kInvalidPSOIndex;
			auto* _pBlob = _cpBuilder.SetShader("Asset/Shader/Compute/TemporalAccumulationShader/TemporalAccumulationShader.cso", "TemporalAccumulationPass", _csIndex);
			_cpBuilder.SetRootSignature(a_pPSOManager, _pBlob);
			_cpBuilder.SetHeapMode(ERGHeapMode::Default);
			_cpBuilder.SetFrameParity(_parity);

			// 依存関係とバインドの宣言（宣言順 = t0～t6）
			_cpBuilder.SrvTable(1)
				.Add("RayGI")
				.Add("GBufferVelocity")
				.Add(_readHistory)
				.Add("Depth")
				.Add("GBufferNormal")
				.Add("PrevDepth")
				.Add("PrevNormal");

			// UAVへの書き込み
			_cpBuilder.BindUAV(2, _writeHistory, DXGI_FORMAT_R8G8B8A8_UNORM, LoadOp::Clear, StoreOp::Store, 0.5f);

			_cpBuilder.ResolveAndCompile(a_pPSOManager);

			// 実行関数 : オプション由来の定数バッファとディスパッチのみ
			_node.executeFunc = [](GraphicsEngine* a_pGE, RenderContext* a_pCtx, const RGPassResources& a_res)
				{
					const auto& _winOp = Option::OptionManager::GetInstance().GetWindowOption();
					auto* _pCmd = a_pCtx->GetCurrentCmdList();

					// 定数バッファ
					struct GITAOp
					{
						float phiDepth;
						float phiNormal;
						float blendRate;
					};
					const auto& _giOp = Option::OptionManager::GetInstance().GetGIOption();
					GITAOp _op = {};
					_op.phiDepth  = _giOp.TAphiDepth;
					_op.phiNormal = _giOp.TAphiNormal;
					_op.blendRate = _giOp.TAblendRate;
					a_pCtx->BindCB()->BindAndAttachDataComputeRootCBV(_pCmd, 0, _op);

					// 実行
					a_pCtx->Dispatch(_winOp.windowWidth / 2 / 8, _winOp.windowHegiht / 2 / 8, 1);
				};

			a_pRegistry->RegisterPass(_node);

			// ------------------------------------------------------------------
			// 2. コピーパス（SpatialDenoise等へ安定したリソース名を提供するため）
			// 宣言だけで完結するので実行関数は不要
			// ------------------------------------------------------------------
			RenderPassNode _copyNode = {};
			_copyNode.name = _isEven ? "TemporalAccumulationCopyPass_Even" : "TemporalAccumulationCopyPass_Odd";
			_copyNode.phase = a_phase;
			RGGlobalsPassBuilder _copyBuilder(&_copyNode);

			_copyBuilder.SetFrameParity(_parity);
			_copyBuilder.Copy(_writeHistory, _finalDst, DXGI_FORMAT_R8G8B8A8_UNORM, LoadOp::Clear, StoreOp::DontCare, 0.5f);

			a_pRegistry->RegisterPass(_copyNode);
		}
	}
}
