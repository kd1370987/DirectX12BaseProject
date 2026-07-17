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
		// 後続のパス（ポストプロセス等）が常に同じ固定名で最新のTAA結果を参照できるようにする宛先
		const std::string _finalDst = "AffterTAAColor";

		// ======================================================================
		// 偶数フレーム用と奇数フレーム用の2つのパスセットを登録する
		// ======================================================================
		for (int i = 0; i < 2; ++i)
		{
			const bool _isEven = (i == 0); // true: 偶数フレーム用, false: 奇数フレーム用
			const ERGFrameParity _parity = _isEven ? ERGFrameParity::Even : ERGFrameParity::Odd;

			// フレームの偶奇に合わせて読み書きするヒストリーバッファの名前を固定化
			const std::string _readHistory  = _isEven ? "TAAHistory_A" : "TAAHistory_B";
			const std::string _writeHistory = _isEven ? "TAAHistory_B" : "TAAHistory_A";

			// ------------------------------------------------------------------
			// TAA パス（コンピュートによる合成）
			// ------------------------------------------------------------------
			RenderPassNode _node = {};
			_node.name = _isEven ? "TAAPass_Even" : "TAAPass_Odd";
			_node.phase = a_phase;
			RGComputePassBuilder _cpBuilder(&_node);

			// シェーダーセット
			uint8_t _csIndex = RenderPassNode::kInvalidPSOIndex;
			auto* _pBlob = _cpBuilder.SetShader("Asset/Shader/Compute/AntiAliasing/TAA/TAA.cso", "TAAShader", _csIndex);
			// ルートシグネチャセット
			_cpBuilder.SetRootSignature(a_pPSOManager, _pBlob);
			_cpBuilder.SetHeapMode(ERGHeapMode::Default);

			// 担当フレームの指定。ランタイムの早期リターンではなくグラフ側の静的な実行条件にする
			_cpBuilder.SetFrameParity(_parity);

			// 依存関係とバインドの宣言（宣言順 = t0～t4）
			_cpBuilder.SrvTable(0)
				.Add("AffterLighting")
				.Add(_readHistory)			// TAA履歴を読み込む
				.Add("GBufferVelocity")
				.Add("Depth")
				.Add("GBufferNormal");

			// 結果を書き込むUAV
			_cpBuilder.BindUAV(1, _writeHistory, DXGI_FORMAT_R8G8B8A8_UNORM, LoadOp::Clear, StoreOp::Store);

			// PSO作成
			_cpBuilder.ResolveAndCompile(a_pPSOManager);

			// 実行関数 : ディスパッチのみ
			_node.executeFunc = [](GraphicsEngine* a_pGE, RenderContext* a_pCtx, const RGPassResources& a_res)
				{
					const auto& _winOp = Option::OptionManager::GetInstance().GetWindowOption();
					a_pCtx->Dispatch(_winOp.windowWidth / 8, _winOp.windowHegiht / 8, 1);
				};
			a_pRegistry->RegisterPass(_node);

			// ------------------------------------------------------------------
			// コピーパス（後続のパスが固定名で参照できるようにする）
			// 宣言だけで完結するので実行関数は不要
			// ------------------------------------------------------------------
			RenderPassNode _copyNode = {};
			_copyNode.name = _isEven ? "TAACopyPass_Even" : "TAACopyPass_Odd";
			_copyNode.phase = a_phase;
			RGGlobalsPassBuilder _copyBuilder(&_copyNode);

			_copyBuilder.SetFrameParity(_parity);
			_copyBuilder.Copy(_writeHistory, _finalDst, DXGI_FORMAT_R8G8B8A8_UNORM);

			a_pRegistry->RegisterPass(_copyNode);
		}
	}
}
