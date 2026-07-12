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
	// ランタイムデータ
	struct RuntimeData
	{
		ID3D12RootSignature* pRootSig;
		uint8_t csIndex;
		D3D12::PipelineStateManager* pPSOManager;

	};
	auto _spPassData = std::make_shared<RuntimeData>();
	_spPassData->pPSOManager = a_pPSOManager;


	// ノードパスビルダー作成
	RenderPassNode _node = {};
	_node.name = "FullRaytracingUpScalePass";
	_node.phase = a_phase;
	RGComputePassBuilder _cpBuilder(&_node);


	// シェーダーセット
	auto* _pBlob = _cpBuilder.SetShader(
		"Asset/Shader/Compute/UpScale/UpScaleCS.cso",
		"FullRaytracingUpScaleShader",
		_spPassData->csIndex
	);
	// ルートシグネチャセット
	_spPassData->pRootSig = _cpBuilder.SetRootSignature(a_pPSOManager, _pBlob);

	// 依存関係構築
	_cpBuilder.ReadSRV("GBufferNormal");
	_cpBuilder.ReadSRV("Depth");
	_cpBuilder.ReadSRV("RayGI");

	_cpBuilder.WriteUAV("FinalFullRay", DXGI_FORMAT_R8G8B8A8_UNORM, LoadOp::Clear, StoreOp::Store);

	// PSO作成
	_cpBuilder.ResolveAndCompile(a_pPSOManager);

	// 実行関数
	_node.executeFunc = [_spPassData](GraphicsEngine* a_pGE, RenderContext* a_pCtx, uint8_t a_passIndex)
		{
			auto* _pCmd = a_pCtx->GetCurrentCmdList();

			// オプション取得
			const auto& _winOp = Option::OptionManager::GetInstance().GetInstance().GetWindowOption();
			// ヒープ / ルートシグネチャ / PSO をセット
			auto* _pPSO = _spPassData->pPSOManager->GetPSO(_spPassData->csIndex);
			a_pCtx->BindHeap();
			a_pCtx->SetComputeRootSignature(_spPassData->pRootSig);
			a_pCtx->SetComputePSO(_pPSO);

			// 定数バッファセット
			struct UpscaleParames
			{
				float scaleRatio;
				float depthSigma;
				float normalPower;
				float pad;
			}; 
			UpscaleParames _params = { 2.0f, 0.5f, 0.5f, 0.0f }; // 2.0f にする
			a_pCtx->BindCB()->BindAndAttachDataComputeRootCBV<UpscaleParames>(_pCmd, 0, _params);

			// SRVセット
			a_pCtx->ComputeBindSRV(1, a_pGE->RefRenderGraph()->GetPassSRV(a_passIndex, "RayGI"));
			a_pCtx->ComputeBindSRV(2, a_pGE->RefRenderGraph()->GetPassSRV(a_passIndex, "Depth"));
			a_pCtx->ComputeBindSRV(3, a_pGE->RefRenderGraph()->GetPassSRV(a_passIndex, "GBufferNormal"));

			// 出力用UAVセット
			a_pCtx->BindUAV(4, a_pGE->RefRenderGraph()->GetPassUAV(a_passIndex, "FinalFullRay"));

			// 実行
			UINT _countX = (_winOp.windowWidth + 7) / 8;
			UINT _countY = (_winOp.windowHegiht + 7) / 8;
			a_pCtx->Dispatch(_countX, _countY, 1);
		};

	a_pRegistry->RegisterPass(_node);
}
