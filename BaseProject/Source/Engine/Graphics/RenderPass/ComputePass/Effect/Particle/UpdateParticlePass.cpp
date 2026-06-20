#include "UpdateParticlePass.h"
#include "Engine/Graphics/RenderGraph/RenderGraph.h"
#include "Engine/Graphics/RenderGraph/RGPassBuilder/RGPassBuilder.h"
#include "Engine/Graphics/GraphicEngine.h"
#include "../../../../../MainEngine.h"
#include "../../../../../Particle/ParticleBufferManager.h"
#include "../../../../../Particle/GPU/GPUParticlePool/GPUParticlePool.h"

#include "Engine/Graphics/RenderContext/RenderContext.h"
#include "Engine/D3D12/PipelineStateManager/PipelineStateManager.h"
#include "Engine/D3D12/D3DObject/CommandList/CommandList.h"

#include "../../../../../Option/OptionManager.h"

namespace Engine::Graphics
{
	void Engine::Graphics::AddUpdateParticlePass(D3D12::PipelineStateManager* a_pPSOManager, RenderGraph& a_rg, const EDrawPhase& a_phase)
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

		// ノードパスビルダー作成
		RenderPassNode _node = {};
		_node.name = "EmitParticlePass";
		RGComputePassBuilder _cpBuilder(&_node, &a_rg);

		// ルートシグネチャセット
		_spPassData->pRootSig = _cpBuilder.SetRootSignature(
			a_pPSOManager,
			"Asset/Shader/Compute/Particle/EmitParticle/EmitParticleShader.cso"
		);
		// シェーダーセット
		_cpBuilder.SetShader(
			"Asset/Shader/Compute/Particle/EmitParticle/EmitParticleShader.cso",
			"EmitParticleShader",
			_spPassData->csIndex
		);

		

		// PSO作成
		_cpBuilder.ResolveAndCompile(a_pPSOManager);

		// 実行関数
		_node.executeFunc = [_spPassData](GraphicsEngine* a_pGE, RenderContext* a_pCtx, uint8_t a_passIndex)
			{
				Editor::MainEditor::Instance().StartWatch("EmitParticlePass");

				// 全プール分回す
				for (auto& [_handle, _pool] : MainEngine::Instance().GetParticleManager()->GetPoolMap())
				{
					// ヒープとルートシグネチャ、PSOをセット
					auto* _pPso = _spPassData->pPSOManager->GetPSO(_spPassData->csIndex);
					a_pCtx->BindHeap();
					a_pCtx->SetComputeRootSignature(_spPassData->pRootSig);
					a_pCtx->SetComputePSO(_pPso);

					// カメラバインド

					// 命令バインド


					// GPUパーティクルプールバインド
					auto _particleUAV = _pool->GetParticlePoolUAV();
					auto _deadListUAV = _pool->GetDeadListUAV();
					auto _counterUAV = _pool->GetCounterUAV();
					a_pCtx->BindUAV(2, {_particleUAV,_deadListUAV,_counterUAV});

					// 出力テクスチャ設定
					a_pCtx->BindUAV(1, _spPassData->pRG->GetUAVCPU("AffterTAAColor"));

					// 実行
					a_pCtx->Dispatch(32, 1, 1);
				}

				Editor::MainEditor::Instance().EndWatch("EmitParticlePass");
			};

		a_rg.AddPassNode(a_phase, _node);
	}
}