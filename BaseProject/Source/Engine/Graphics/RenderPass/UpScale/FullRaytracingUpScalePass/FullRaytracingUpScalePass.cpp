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
	// ルートパラメータ : 0=カメラCB(b0) / 1=パラメータCB(b1) / 2〜4=SRV / 5=UAV
	_cpBuilder.BindSRV(2, "FinalGI");
	_cpBuilder.BindSRV(3, "Depth");
	_cpBuilder.BindSRV(4, "GBufferNormal");

	// 出力用UAV
	// HDR : ライティングが読むGIもR16Fのまま渡す
	_cpBuilder.BindUAV(5, "FinalFullRay", DXGI_FORMAT_R16G16B16A16_FLOAT, LoadOp::Clear, StoreOp::Store);

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
			// depthSigma : ビュー深度に対する相対値。0.05 = 距離の5%まで同じ面とみなす
			// normalPower: pow()の指数。0.5(=sqrt)ではエッジ判定がほぼ効かないので上げる
			UpscaleParames _params = { 2.0f, 0.05f, 32.0f, 0.0f };

			// カメラCB(b0) : シェーダ側でワールド座標を復元してエッジ判定に使う
			a_pCtx->ComputeBindRootCBV(0, a_pGE->GetCameraData());

			// パラメータCB(b1)
			a_pCtx->BindCB()->BindAndAttachDataComputeRootCBV<UpscaleParames>(_pCmd, 1, _params);

			// 実行
			a_pCtx->Dispatch((_winOp.windowWidth + 7) / 8, (_winOp.windowHeight + 7) / 8, 1);
		};

	a_pRegistry->RegisterPass(_node);
}
