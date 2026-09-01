#include "ParticleSimulation.h"

#include "Engine/Graphics/GraphicEngine.h"
#include "Engine/MainEngine.h"
#include "Engine/Particle/ParticleBufferManager.h"
#include "Engine/Particle/GPU/GPUParticlePool/GPUParticlePool.h"

#include "Engine/Graphics/RenderContext/RenderContext.h"
#include "Engine/D3D12/PipelineStateManager/PipelineStateManager.h"

#include "Engine/Resource/Data/Shader/IO/ShaderIO.h"
#include "Engine/Resource/Manager/ResourceManager/ResourceManager.h"

namespace
{
	//======================================================================================
	// 用意しておくもの
	//
	// 旧 EmitParticlePass / UpdateParticlePass が
	// RGComputePassBuilder に作らせていたぶんをここへ移した
	//======================================================================================
	struct ParticleRuntime
	{
		Engine::D3D12::PipelineStateManager* pPSOManager = nullptr;

		Engine::Handle<ID3D12RootSignature> emitRootSig = {};
		uint8_t emitCSIndex = 255;

		Engine::Handle<ID3D12RootSignature> updateRootSig = {};
		uint8_t updateCSIndex = 255;

		// このフレームの乱数の種
		uint32_t frameCounter = 0;
	};
	ParticleRuntime g_particle = {};

	// シェーダーからルートシグネチャとコンピュートPSOを起こす
	bool SetupComputeShader(
		Engine::D3D12::PipelineStateManager* a_pPSOManager,
		const std::string& a_csPath,
		const std::string& a_psoName,
		Engine::Handle<ID3D12RootSignature>& a_outRootSig,
		uint8_t& a_outCSIndex)
	{
		using namespace Engine;

		auto _csHandle = Resource::ShaderIO::Request(a_csPath);
		auto* _pShader = Resource::ResourceManager::Instance().Ref(_csHandle);
		if (!_pShader || !_pShader->Get()) return false;

		a_outRootSig = a_pPSOManager->Request(_pShader->Get());
		if (!a_outRootSig.IsValid()) return false;

		D3D12::ComputePipelineDesc _desc = {};
		_desc.SetName(a_psoName);
		_desc.desc.CS.pShaderBytecode = _pShader->Get()->GetBufferPointer();
		_desc.desc.CS.BytecodeLength = _pShader->Get()->GetBufferSize();
		_desc.SetRootSignature(a_pPSOManager->GetRootSignature(a_outRootSig));

		a_outCSIndex = static_cast<uint8_t>(a_pPSOManager->RequestHandle(_desc).GetIndex());
		return true;
	}
}

namespace Engine::Graphics
{
	void SetupParticleSimulation(D3D12::PipelineStateManager* a_pPSOManager)
	{
		if (!a_pPSOManager) return;
		g_particle.pPSOManager = a_pPSOManager;

		SetupComputeShader(
			a_pPSOManager,
			"Asset/Shader/Source/Particle/Emit/EmitParticleShaeder.cso",
			"EmitParticleShader",
			g_particle.emitRootSig, g_particle.emitCSIndex);

		SetupComputeShader(
			a_pPSOManager,
			"Asset/Shader/Source/Particle/Update/UpdateParticleShader.cso",
			"UpdateParticleShader",
			g_particle.updateRootSig, g_particle.updateCSIndex);
	}

	void ExecuteParticleSimulation(GraphicsEngine* a_pGE, RenderContext* a_pCtx)
	{
		if (!a_pGE || !a_pCtx) return;
		if (!g_particle.pPSOManager) return;

		auto* _pCmd = a_pCtx->GetCurrentCmdList();
		if (!_pCmd) return;

		auto* _pParticleManager = MainEngine::Instance().GetParticleManager();
		if (!_pParticleManager) return;

		//----------------------------------------------------------------------------------
		// 発生
		//----------------------------------------------------------------------------------
		// このフレームの乱数の種を進める
		++g_particle.frameCounter;

		for (auto& [_handle, _pool] : _pParticleManager->GetPoolMap())
		{
			if (!_pool) continue;
			// プールが読み込み済みかチェック
			if (!MainEngine::Instance().RefParticleManager()->IsLoaded(_handle)) continue;

			// このフレームに発生命令が無いなら何もしない
			auto _requests = _pParticleManager->GetRequests(_handle);
			if (_requests.empty()) continue;

			// ヒープとルートシグネチャ、PSOをセット
			a_pCtx->BindHeap();
			a_pCtx->SetComputeRootSignature(g_particle.emitRootSig);
			a_pCtx->SetComputePSO(g_particle.pPSOManager->GetPSO(g_particle.emitCSIndex));

			// 命令バインド
			const auto* _pEmitBuff = _pParticleManager->GetEmitBuffer(_handle);
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
			// でたらめな emitCount でプールを食いつぶすので必ず切り詰める
			_cbEmit.requestCount = static_cast<uint32_t>(
				(std::min)(_requests.size(), _pEmitBuff->GetElementNum())
			);
			if (_cbEmit.requestCount == 0) continue;

			// プールごとにも種をずらす(同一フレームに複数プールが出しても被らないように)
			_cbEmit.frameSeed = g_particle.frameCounter * 2654435761u + _handle.id;
			a_pCtx->BindCB()->BindAndAttachDataComputeRootCBV<EmitCB>(_pCmd, 0, _cbEmit);

			// GPUパーティクルプールバインド
			a_pCtx->BindUAV(2, { _pool->GetParticlePoolUAV(), _pool->GetDeadListUAV(), _pool->GetCounterUAV() });

			// 実行
			// 1スレッド = エミット命令1つ なので、必要なのは命令数分だけ
			const UINT _dispatchNum = (_cbEmit.requestCount + 31u) / 32u;
			a_pCtx->Dispatch(_dispatchNum, 1, 1);

			// ★UAVバリア必須。
			// この下の更新は同じ deadList / counter を触る。
			// バリアが無いと2つのDispatchがGPU上で並列に走り、
			// 更新側の「返却(counter++ してから deadList[count] へ書く)」と
			// 発生側の「取り出し(counter-- してから deadList[count-1] を読む)」が
			// 交錯する。書き込み前のスロットを読んでしまうと、
			// ・使用中インデックスを二重に掴む(生きている粒が上書きされて消える)
			// ・返却したインデックスがカウンターの上に取り残されて二度と拾われない
			// が起き、空きスロットが少しずつ減り続けてエミット数が先細りする
			D3D12::UAVBarrier(
				_pCmd,
				{
					_pool->GetParticlePoolResource(),
					_pool->GetDeadListResource(),
					_pool->GetCounterResource()
				}
			);
		}

		//----------------------------------------------------------------------------------
		// 更新
		//----------------------------------------------------------------------------------
		for (auto& [_handle, _pool] : _pParticleManager->GetPoolMap())
		{
			if (!_pool) continue;
			if (!MainEngine::Instance().RefParticleManager()->IsLoaded(_handle)) continue;

			// ヒープとルートシグネチャ、PSOをセット
			a_pCtx->BindHeap();
			a_pCtx->SetComputeRootSignature(g_particle.updateRootSig);
			a_pCtx->SetComputePSO(g_particle.pPSOManager->GetPSO(g_particle.updateCSIndex));

			// 更新設定バインド
			// ※ HLSL 側 UpdateCB(UpdateParticleShader.hlsl)と並びを合わせること
			struct UpdateCB
			{
				float deltaTime;
				DirectX::XMFLOAT3 gravity;

				float drag;
				DirectX::XMFLOAT3 pad;
			};
			// 固定値だと実フレームレートと寿命の減りが一致しない(重いほど長生きする)ため
			// 実際の経過時間を渡す
			UpdateCB _cbData = {};
			_cbData.deltaTime = MainEngine::Instance().GetDeltaTime();

			// 重力と減衰はアセット単位。プールごとに回しているのでここで引ける
			if (const auto* _pParticle = Resource::ResourceManager::Instance().Get(_handle))
			{
				// GravityPow は「重力をどれだけ受けるか」の倍率。
				// 1 で普通に落ち、0 で無重力、負にすると浮き上がる(煙向き)
				constexpr float _kGravity = 9.81f;
				_cbData.gravity = { 0.0f, -_kGravity * _pParticle->GetGravityPow(), 0.0f };

				_cbData.drag = _pParticle->GetDrag();
			}

			a_pCtx->BindCB()->BindAndAttachDataComputeRootCBV<UpdateCB>(_pCmd, 0, _cbData);

			// 命令バインド
			if (const auto* _pEmitBuff = _pParticleManager->GetEmitBuffer(_handle))
			{
				a_pCtx->ComputeBindSRV(1, _pEmitBuff->GetSRVHandle());
			}

			// GPUパーティクルプールバインド
			a_pCtx->BindUAV(2, { _pool->GetParticlePoolUAV(), _pool->GetDeadListUAV(), _pool->GetCounterUAV() });

			// 実行
			// 切り上げること。切り捨てると容量が32の倍数でない場合に
			// 末尾のパーティクルが一度も更新されず、寿命が減らないまま残り続ける。
			// (デッドリストは末尾から取り出すので、最初に発生した粒がまさにそこに入る)
			// シェーダー側は GetDimensions で範囲外スレッドを弾いているため多い分は安全
			const UINT _threadNum = _pool->GetMaxCapacity();
			const UINT _dispatchNum = (_threadNum + 31u) / 32u;
			a_pCtx->Dispatch(_dispatchNum, 1, 1);

			// ★UAVバリア必須。
			// 次フレームの発生が同じ deadList / counter から取り出す。
			// D3D12 は同一キューでもバリアが無ければ Dispatch 同士が重なって走れるため、
			// (コマンドリストをまたいでも)ここで区切らないと
			// このフレームの返却が終わる前に次の取り出しが走り、
			// 空きスロットが取りこぼされて減り続ける
			D3D12::UAVBarrier(
				_pCmd,
				{
					_pool->GetParticlePoolResource(),
					_pool->GetDeadListResource(),
					_pool->GetCounterResource()
				}
			);
		}
	}
}
