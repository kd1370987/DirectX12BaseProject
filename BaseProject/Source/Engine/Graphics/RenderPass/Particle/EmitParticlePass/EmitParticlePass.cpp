#include "EmitParticlePass.h"

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

namespace Engine::Graphics
{

	void AddEmitParticlePass(D3D12::PipelineStateManager* a_pPSOManager, RenderPassRegistry* a_pRegistry, const EDrawPhase& a_phase)
	{
		// ランタイムデータ
		struct RuntimeData
		{
			ID3D12RootSignature* pRootSig;
			uint8_t csIndex;
			D3D12::PipelineStateManager* pPSOManager;

			// 乱数の種。毎フレーム変えないとシェーダー側が
			// 毎回まったく同じパーティクルを作ってしまう
			uint32_t frameCounter = 0;
		};
		auto _spPassData = std::make_shared<RuntimeData>();
		_spPassData->pPSOManager = a_pPSOManager;
		

		// ノードパスビルダー作成
		RenderPassNode _node = {};
		_node.name = "EmitParticlePass";
		_node.phase = a_phase;
		RGComputePassBuilder _cpBuilder(&_node);


		// シェーダーセット
		auto* _pBlob = _cpBuilder.SetShader(
			"Asset/Shader/Compute/Particle/EmitParticle/EmitParticleShaeder.cso",
			"EmitParticleShader",
			_spPassData->csIndex
		);
		// ルートシグネチャセット
		_spPassData->pRootSig = _cpBuilder.SetRootSignature(a_pPSOManager,_pBlob);

		// PSO作成
		_cpBuilder.ResolveAndCompile(a_pPSOManager);

		// 実行関数
		_node.executeFunc = [_spPassData](GraphicsEngine* a_pGE, RenderContext* a_pCtx, const RGPassResources& a_res)
			{
				// このフレームの乱数の種を進める
				++_spPassData->frameCounter;

				// 全プール分回す
				for (auto& [_handle, _pool] : MainEngine::Instance().GetParticleManager()->GetPoolMap())
				{
					if (!_pool) continue;
					// プールが読み込み済みかチェック
					if (!MainEngine::Instance().RefParticleManager()->IsLoaded(_handle)) continue;

					// このフレームに発生命令が無いなら何もしない。
					// (命令0でもDispatchしていたため、ログ上は毎フレーム動いているように見えていた)
					auto _requests = MainEngine::Instance().GetParticleManager()->GetRequests(_handle);
					if (_requests.empty()) continue;

					// ヒープとルートシグネチャ、PSOをセット
					auto* _pPso = _spPassData->pPSOManager->GetPSO(_spPassData->csIndex);
					a_pCtx->BindHeap();
					a_pCtx->SetComputeRootSignature(_spPassData->pRootSig);
					a_pCtx->SetComputePSO(_pPso);

					auto* _pCmd = a_pCtx->GetCurrentCmdList();

					struct EmitCB
					{
						uint32_t requestCount;
						uint32_t frameSeed;
					};
					EmitCB _cbEmit = {};
					_cbEmit.requestCount = static_cast<uint32_t>(_requests.size());

					// プールごとにも種をずらす(同一フレームに複数プールが出しても被らないように)
					_cbEmit.frameSeed = _spPassData->frameCounter * 2654435761u + _handle.id;
					a_pCtx->BindCB()->BindAndAttachDataComputeRootCBV<EmitCB>(
						_pCmd,
						0,
						_cbEmit
					);

					// 命令バインド
					const auto* _pEmitBuff = MainEngine::Instance().GetParticleManager()->GetEmitBuffer(_handle);
					if (_pEmitBuff)
					{
						a_pCtx->ComputeBindSRV(1, _pEmitBuff->GetSRVHandle());
					}

					// GPUパーティクルプールバインド
					auto _particleUAV = _pool->GetParticlePoolUAV();
					auto _deadListUAV = _pool->GetDeadListUAV();
					auto _counterUAV = _pool->GetCounterUAV();
					a_pCtx->BindUAV(2, { _particleUAV,_deadListUAV,_counterUAV });

					// 実行
					UINT _dispatchNum = static_cast<UINT>(_pool->GetMaxCapacity() / 32u);
					a_pCtx->Dispatch(_dispatchNum, 1, 1);
					ENGINE_LOG("ParticleEmitPass : 実行 命令数=%u", _cbEmit.requestCount);
				}
			};

		a_pRegistry->RegisterPass(_node);
	}
}
