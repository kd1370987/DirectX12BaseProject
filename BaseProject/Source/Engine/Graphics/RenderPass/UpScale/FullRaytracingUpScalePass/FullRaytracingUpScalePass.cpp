#include "FullRaytracingUpScalePass.h"

#include "Engine/Graphics/RenderPassRegistry/RenderPassRegistry.h"
#include "Engine/Graphics/RenderGraph/RenderGraph.h"
#include "Engine/Graphics/RenderGraph/RGPassBuilder/RGPassBuilder.h"
#include "Engine/Graphics/GraphicEngine.h"
#include "Engine/MainEngine.h"
#include "Engine/Particle/ParticleBufferManager.h"
#include "Engine/Particle/GPU/GPUParticlePool/GPUParticlePool.h"

#include "Engine/Graphics/RenderContext/RenderContext.h"
#include "Engine/D3D12/PipelineStateManager/PipelineStateManager.h"

#include "Engine/Option/OptionManager.h"

void Engine::Graphics::AddFullRaytracingUpScalePass(
	D3D12::PipelineStateManager* a_pPSOManager, RenderPassRegistry* a_pRegistry, const EDrawPhase& a_phase
)
{
	// ノードパスビルダー作成
	RenderPassNode _node = {};
	_node.name = "FullRaytracingUpScalePass";
	_node.phase = a_phase;
	RGComputePassBuilder _cpBuilder(&_node);

	// シェーダーセット
	uint8_t _csIndex = RenderPassNode::kInvalidPSOIndex;
	auto* _pBlob = _cpBuilder.SetShader(
		"Asset/Shader/Compute/UpScale/UpScaleCS.cso",
		"FullRaytracingUpScaleShader",
		_csIndex
	);
	// ルートシグネチャセット
	_cpBuilder.SetRootSignature(a_pPSOManager, _pBlob);
	_cpBuilder.SetHeapMode(ERGHeapMode::Default);

	// 依存関係とバインドの宣言。このパスはSRVを個別のルートパラメータへ張る
	_cpBuilder.BindSRV(1, "FinalGI");
	_cpBuilder.BindSRV(2, "Depth");
	_cpBuilder.BindSRV(3, "GBufferNormal");

	// 出力用UAV
	_cpBuilder.BindUAV(4, "FinalFullRay", DXGI_FORMAT_R8G8B8A8_UNORM, LoadOp::Clear, StoreOp::Store);

	// PSO作成
	_cpBuilder.ResolveAndCompile(a_pPSOManager);

	// 実行関数 : 定数バッファとディスパッチのみ
	_node.executeFunc = [](GraphicsEngine* a_pGE, RenderContext* a_pCtx, const RGPassResources& a_res)
		{
			const auto& _winOp = Option::OptionManager::GetInstance().GetWindowOption();
			auto* _pCmd = a_pCtx->GetCurrentCmdList();

			// 定数バッファセット
			struct UpscaleParames
			{
				float scaleRatio;
				float depthSigma;
				float normalPower;
				float pad;
			};
			UpscaleParames _params = { 2.0f, 0.5f, 0.5f, 0.0f };
			a_pCtx->BindCB()->BindAndAttachDataComputeRootCBV<UpscaleParames>(_pCmd, 0, _params);

			// 実行
			a_pCtx->Dispatch((_winOp.windowWidth + 7) / 8, (_winOp.windowHegiht + 7) / 8, 1);
		};

	a_pRegistry->RegisterPass(_node);
}
