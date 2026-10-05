#include "ParticleSimulation.h"

#include "Engine/Graphics/GraphicsEngine.h"
#include "Engine/MainEngine.h"
#include "Engine/Graphics/Particle/ParticleBufferManager.h"
#include "Engine/Graphics/Particle/GPU/GPUParticlePool/GPUParticlePool.h"
#include "../../../Graphics/Particle/GPU/EmitterSlotPool/EmitterSlotPool.h"

#include "Engine/Graphics/Frame/RenderContext/RenderContext.h"
#include "Engine/Graphics/PipelineState/PipelineStateManager/PipelineStateManager.h"

#include "Engine/Resource/Data/Shader/IO/ShaderIO.h"
#include "Engine/Resource/Manager/ResourceManager/ResourceManager.h"
#include "Engine/Resource/Data/QuadPolygon/QuadPolygon.h"

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
		Engine::Graphics::PipelineStateManager* pPSOManager = nullptr;
		Engine::Resource::ResourceManager* pResourceManager = nullptr;	// アセットの値を引く(借り物)

		// 各シェーダーに対応する組み合わせ
		Engine::Handle<ID3D12RootSignature> emitRootSig = {};
		Engine::Handle<ID3D12PipelineState> emitPSO = {};

		Engine::Handle<ID3D12RootSignature> updateRootSig = {};
		Engine::Handle<ID3D12PipelineState> updatePSO = {};

		Engine::Handle<ID3D12RootSignature> resetRootSig = {};
		Engine::Handle<ID3D12PipelineState> resetPSO = {};

		// このフレームの乱数の種
		uint32_t frameCounter = 0;
	};
	ParticleRuntime g_particle = {};

	// シェーダーからルートシグネチャとコンピュートPSOを起こす
	bool SetupComputeShader(
		Engine::Graphics::PipelineStateManager* a_pPSOManager,
		Engine::Resource::ResourceManager& a_resourceManager,
		const std::string& a_csPath,
		const std::string& a_psoName,
		Engine::Handle<ID3D12RootSignature>& a_outRootSig,
		Engine::Handle<ID3D12PipelineState>& a_outPSO)
	{
		using namespace Engine;

		auto _csHandle = Resource::ShaderIO::Request(a_resourceManager, a_csPath);
		auto* _pShader = a_resourceManager.Ref(_csHandle);
		if (!_pShader || !_pShader->Get()) return false;

		a_outRootSig = a_pPSOManager->Request(_pShader->Get());
		if (!a_outRootSig.IsValid()) return false;

		D3D12::ComputePipelineDesc _desc = {};
		_desc.SetName(a_psoName);
		_desc.desc.CS.pShaderBytecode = _pShader->Get()->GetBufferPointer();
		_desc.desc.CS.BytecodeLength = _pShader->Get()->GetBufferSize();
		_desc.SetRootSignature(a_pPSOManager->GetRootSignature(a_outRootSig));

		a_outPSO = a_pPSOManager->RequestHandle(_desc);
		return a_outPSO.IsValid();
	}
}

namespace Engine::Graphics
{
	void SetupParticleSimulation(PipelineStateManager* a_pPSOManager, Resource::ResourceManager& a_resourceManager)
	{
		if (!a_pPSOManager) return;
		g_particle.pPSOManager = a_pPSOManager;
		g_particle.pResourceManager = &a_resourceManager;

		SetupComputeShader(
			a_pPSOManager,
			a_resourceManager,
			"Asset/Shader/Source/Particle/Emit/EmitParticleShaeder.cso",
			"EmitParticleShader",
			g_particle.emitRootSig, g_particle.emitPSO);

		SetupComputeShader(
			a_pPSOManager,
			a_resourceManager,
			"Asset/Shader/Source/Particle/Update/UpdateParticleShader.cso",
			"UpdateParticleShader",
			g_particle.updateRootSig, g_particle.updatePSO);

		SetupComputeShader(
			a_pPSOManager,
			a_resourceManager,
			"Asset/Shader/Source/Particle/Update/ResetDrawArgs.cso",
			"ResetDrawArgsShader",
			g_particle.resetRootSig, g_particle.resetPSO);
	}

	void ExecuteParticleSimulation(GraphicsEngine* a_pGE, RenderContext* a_pCtx)
	{
		if (!a_pGE || !a_pCtx) return;
		if (!g_particle.pPSOManager) return;

		auto* _pCmd = a_pCtx->GetCurrentCmdList();
		if (!_pCmd) return;

		auto* _pParticleManager = a_pGE->RefParticleManager();
		if (!_pParticleManager) return;

		//----------------------------------------------------------------------------------
		// 発生
		//----------------------------------------------------------------------------------
		// このフレームの乱数の種を進める
		++g_particle.frameCounter;

		//----------------------------------------------------------------------------------
		// 発生命令は全プール共通の1本(UploadEmitData がつなげて送ったもの)。
		// プールごとに「どこからどこまでか」を引いて、そこだけを読ませる。
		// まだ一度も送っていなければ、出す命令はどこにも無い
		//----------------------------------------------------------------------------------
		const auto* _pEmitBuff = _pParticleManager->GetEmitterBuffer();
		const UINT _emitIndex = _pEmitBuff ? static_cast<UINT>(_pEmitBuff->GetSRVHandle().GetIndex()) : 0xFFFFFFFFu;

		for (auto& [_handle, _pool] : _pParticleManager->GetPoolMap())
		{
			if (!_pool || !_pEmitBuff) continue;
			// プールが読み込み済みかチェック
			if (!_pParticleManager->IsReady(_handle)) continue;

			// このフレームに発生命令が無いなら何もしない
			const auto _range = _pParticleManager->GetEmitRange(_handle);
			if (_range.count == 0) continue;

			// ヒープとルートシグネチャ、PSOをセット
			a_pCtx->BindBindlessHeaps();
			a_pCtx->SetComputeRootSignature(g_particle.emitRootSig);
			a_pCtx->SetComputePSO(g_particle.pPSOManager->GetPSO(g_particle.emitPSO));

			// 命令バインド(バインドレス : 番号をルート定数で渡す)
			a_pCtx->ComputeBindDescriptorIndices(1, std::span<const UINT>(&_emitIndex, 1));

			// ※ HLSL 側 ParticleEmitSetting(Common/RootParameters/Particle.hlsli)と並びを合わせること
			struct EmitCB
			{
				uint32_t requestOffset;	// 共通の1本の中で、このプールの命令が始まる位置
				uint32_t requestCount;	// このプールの命令の数
				uint32_t frameSeed;		// フレームごとに変わる乱数の種
				uint32_t pad;
			};
			EmitCB _cbEmit = {};

			// 範囲は UploadEmitData が実際に送ったぶんだけなので、そのまま渡してよい
			_cbEmit.requestOffset = _range.offset;
			_cbEmit.requestCount = _range.count;

			// プールごとにも種をずらす(同一フレームに複数プールが出しても被らないように)
			_cbEmit.frameSeed = g_particle.frameCounter * 2654435761u + _handle.id;
			a_pCtx->BindCB()->BindAndAttachDataComputeRootCBV<EmitCB>(_pCmd, 0, _cbEmit);

			// GPUパーティクルプールバインド : 本体 / デッドリスト / カウンターの順(シェーダーの u0-u2 と同じ)
			const UINT _poolIndices[] = {
				_pool->GetParticlePoolUAV().GetIndex(),
				_pool->GetDeadListUAV().GetIndex(),
				_pool->GetCounterUAV().GetIndex(),
			};
			a_pCtx->ComputeBindDescriptorIndices(2, _poolIndices);

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
		//
		// 発生源の席は全プール共通の1本。まだ一度も転送されていなければ
		// 引けない番号で読ませないよう、更新ごと見送る
		// (席 0 は初期化で取っているので、最初のフレームの転送で必ずできる)
		//----------------------------------------------------------------------------------
		const auto* _pSlotPool = _pParticleManager->GetEmitterSlotPool();
		if (!_pSlotPool) return;
		const UINT _slotSRVIndex = _pSlotPool->GetSRVIndex();
		if (_slotSRVIndex == (std::numeric_limits<UINT>::max)()) return;

		// 間接描画の引数をリセットできるか。
		// 更新シェーダーは生き残った粒を数えて引数へ足すので、リセットできないと
		// 前のフレームの数に足し続けることになる。リセット用シェーダーが読めていないときは
		// 更新ごと見送る(印が立たないので描画もされない)
		const bool _isResetReady = g_particle.resetRootSig.IsValid() && g_particle.resetPSO.IsValid();
		if (!_isResetReady) return;

		// 描く板ポリのインデックス数。ParticlePass が張るもの(平らな1枚板)と合わせる
		const UINT _quadIndexCount = a_pGE->RefQuadPolygon() ? a_pGE->RefQuadPolygon()->GetIndexCount() : 0u;

		for (auto& [_handle, _pool] : _pParticleManager->GetPoolMap())
		{
			if (!_pool) continue;
			if (!_pParticleManager->IsReady(_handle)) continue;

			// 眠っているプール(最後に出してから最大寿命が経った)には生きている粒が無い。
			// 更新は容量ぶん走るので、回さずに飛ばす
			if (!_pParticleManager->IsAwake(_handle)) continue;

			// ヒープを先に張る。
			// ルートシグネチャが CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED のときは、
			// SetComputeRootSignature より前に SetDescriptorHeaps を済ませておく決まり
			// (このフレームに発生命令が無いと、上の発生ループで張られていない)
			a_pCtx->BindBindlessHeaps();

			// 引数バッファと生存リストが無ければ、更新シェーダーの書き先が無い
			if (!_pool->GetDrawArgsResource() || !_pool->RefAliveList().GetResource()) continue;

			//------------------------------------------------------------------
			// 間接描画の引数をリセットする
			//
			// フレームの頭は COMMON(前のフレームの終わりで FinishFrame が戻す)なので、書く前に UAV へ。
			// 生存リストも Update が書くので一緒に UAV へ。
			// インスタンス数は 0 から始め、Update が生き残った粒を数えて足す。
			// インデックス数は描く板ポリ(ParticlePass が張るもの)と合わせる
			//------------------------------------------------------------------
			_pool->RefDrawArgs().Barrier(_pCmd, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
			_pool->RefAliveList().Barrier(_pCmd, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

			a_pCtx->SetComputeRootSignature(g_particle.resetRootSig);
			a_pCtx->SetComputePSO(g_particle.resetPSO);
			const UINT _resetParam[] = {
				_pool->RefDrawArgs().GetUAV().GetIndex(),	// 引数バッファの UAV 番号
				_quadIndexCount,							// 板ポリのインデックス数(1枚板なら 6)
				0u,											// インスタンス数(Update が数えて足す)
			};
			a_pCtx->ComputeBindDescriptorIndices(0, _resetParam);
			a_pCtx->Dispatch(1, 1, 1);
			D3D12::UAVBarrier(_pCmd, { _pool->RefDrawArgs().GetResource() });

			// ルートシグネチャ、PSOをセット
			a_pCtx->SetComputeRootSignature(g_particle.updateRootSig);
			a_pCtx->SetComputePSO(g_particle.pPSOManager->GetPSO(g_particle.updatePSO));

			// 更新設定バインド
			// ※ HLSL 側 UpdateCB(UpdateParticleShader.hlsl)と並びを合わせること
			struct UpdateCB
			{
				float deltaTime;
				Math::Vector3 gravity;

				float drag;
				Math::Vector3 pad;
			};
			// 固定値だと実フレームレートと寿命の減りが一致しない(重いほど長生きする)ため
			// 実際の経過時間を渡す
			UpdateCB _cbData = {};
			_cbData.deltaTime = MainEngine::Instance().GetDeltaTime();

			// 重力と減衰はアセット単位。プールごとに回しているのでここで引ける
			if (const auto* _pParticle = g_particle.pResourceManager ? g_particle.pResourceManager->Get(_handle) : nullptr)
			{
				// GravityPow は「重力をどれだけ受けるか」の倍率。
				// 1 で普通に落ち、0 で無重力、負にすると浮き上がる(煙向き)
				constexpr float _kGravity = 9.81f;
				_cbData.gravity = { 0.0f, -_kGravity * _pParticle->GetGravityPow(), 0.0f };

				_cbData.drag = _pParticle->GetDrag();
			}

			a_pCtx->BindCB()->BindAndAttachDataComputeRootCBV<UpdateCB>(_pCmd, 0, _cbData);

			// 命令と発生源の席のバインド。並びはシェーダーの PassDescriptorIndex0(命令 → 席)と同じ。
			// 更新シェーダーは発生命令を読まないが、ルートシグネチャの席は埋めておく(共通の1本の番号)。
			// 発生源の席はローカル空間の粒に重力を掛けるときに読む(席の回転で重力をローカルへ回す)
			{
				const UINT _updateIndices[] = {
					_emitIndex,
					_slotSRVIndex,
				};
				a_pCtx->ComputeBindDescriptorIndices(1, _updateIndices);
			}

			// GPUパーティクルプールバインド。並びはシェーダーの PassDescriptorIndex1 と同じ :
			// 本体 / デッドリスト / カウンター / 生存リスト / 間接描画の引数
			// (発生シェーダーは前の3つだけ。こちらは生き残りを積む先と数える先も渡す)
			const UINT _poolIndices[] = {
				_pool->GetParticlePoolUAV().GetIndex(),
				_pool->GetDeadListUAV().GetIndex(),
				_pool->GetCounterUAV().GetIndex(),
				_pool->GetAliveListUAV().GetIndex(),
				_pool->RefDrawArgs().GetUAV().GetIndex(),
			};
			a_pCtx->ComputeBindDescriptorIndices(2, _poolIndices);

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

			// 描画で読むので、引数は間接引数の状態へ、生存リストは VS が SRV で読む状態へ。
			// 遷移のバリアが Update の書き込みの完了待ちも兼ねる。
			// 印を立てたプールだけが描かれ、フレームの終わりで COMMON へ戻される
			_pool->RefDrawArgs().Barrier(_pCmd, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
			_pool->RefAliveList().Barrier(_pCmd, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
			_pool->SetArgsReady(true);
		}
	}
}
