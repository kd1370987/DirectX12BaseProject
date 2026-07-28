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

					// 命令バインド
					const auto* _pEmitBuff = MainEngine::Instance().GetParticleManager()->GetEmitBuffer(_handle);
					if (!_pEmitBuff) continue;
					a_pCtx->ComputeBindSRV(1, _pEmitBuff->GetSRVHandle());

					struct EmitCB
					{
						uint32_t requestCount;
						uint32_t frameSeed;
					};
					EmitCB _cbEmit = {};

					// 命令バッファの要素数を超えた分は転送されていない。
					// そのまま渡すとシェーダーが未初期化領域を EmitData として読み、
					// でたらめな emitCount でプールを食いつぶすので必ず切り詰める。
					_cbEmit.requestCount = static_cast<uint32_t>(
						(std::min)(_requests.size(), _pEmitBuff->GetElementNum())
					);
					if (_cbEmit.requestCount == 0) continue;

					// プールごとにも種をずらす(同一フレームに複数プールが出しても被らないように)
					_cbEmit.frameSeed = _spPassData->frameCounter * 2654435761u + _handle.id;
					a_pCtx->BindCB()->BindAndAttachDataComputeRootCBV<EmitCB>(
						_pCmd,
						0,
						_cbEmit
					);

					// GPUパーティクルプールバインド
					auto _particleUAV = _pool->GetParticlePoolUAV();
					auto _deadListUAV = _pool->GetDeadListUAV();
					auto _counterUAV = _pool->GetCounterUAV();
					a_pCtx->BindUAV(2, { _particleUAV,_deadListUAV,_counterUAV });

					// 実行
					// 1スレッド = エミット命令1つ なので、必要なのは命令数分だけ。
					// (プール容量から求めるのは無駄打ちなうえ、切り捨てで足りなくなる)
					UINT _dispatchNum = (_cbEmit.requestCount + 31u) / 32u;
					a_pCtx->Dispatch(_dispatchNum, 1, 1);
					ENGINE_LOG("ParticleEmitPass : 実行 命令数=%u", _cbEmit.requestCount);

					// ★UAVバリア必須。
					// このあとの UpdateParticlePass は同じ deadList / counter を触る。
					// バリアが無いと2つのDispatchがGPU上で並列に走り、
					// Update側の「返却(counter++ してから deadList[count] へ書く)」と
					// Emit側の「取り出し(counter-- してから deadList[count-1] を読む)」が
					// 交錯する。書き込み前のスロットを読んでしまうと、
					// ・使用中インデックスを二重に掴む(生きている粒が上書きされて消える)
					// ・返却したインデックスがカウンターの上に取り残されて二度と拾われない
					// が起き、空きスロットが少しずつ減り続けてエミット数が先細りする。
					D3D12::UAVBarrier(
						_pCmd,
						{
							_pool->GetParticlePoolResource(),
							_pool->GetDeadListResource(),
							_pool->GetCounterResource()
						}
					);
				}
			};

		a_pRegistry->RegisterPass(_node);
	}
}
