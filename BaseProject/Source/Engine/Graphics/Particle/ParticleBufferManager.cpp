#include "ParticleBufferManager.h"

#include "../../Resource/Manager/AssetDatabase/AssetDatabase.h"
#include "../GraphicsEngine.h"
#include "../../Resource/Manager/ResourceManager/ResourceManager.h"

#include "GPU/EmitterSlotPool/EmitterSlotPool.h"

#include "Engine/MainEngine.h"	// 作り直した命令バッファの遅延解放(RegisterDeferredResource)

namespace Engine::Particle
{
	ParticleBufferManager::ParticleBufferManager()
	{}
	ParticleBufferManager::~ParticleBufferManager()
	{}

	void Engine::Particle::ParticleBufferManager::Init(
		Graphics::GraphicsEngine* a_pGraphicsEngine,
		D3D12::DescriptorHeapManager* a_pHeapManager,
		D3D12::GraphicsCommandList* a_pCmdList
	)
	{
		// ビューの置き場と転送の依頼先を控える : プールは非同期に作られるので、そこまで持ち回る
		m_pHeapManager = a_pHeapManager;
		m_pGraphicsEngine = a_pGraphicsEngine;

		// パーティクルアセット座席管理クラスの初期化
		// (Release の後にもう一度 Init されても席 0 を取り直せるよう、Init は毎回通す)
		if (!m_upEmitterSlotPool)
		{
			m_upEmitterSlotPool = std::make_unique<EmitterSlotPool>();
		}
		m_upEmitterSlotPool->Init(EMITTER_SLOT_BLOCK_SIZE);
	}
	void ParticleBufferManager::Release()
	{
		// 非同期ロード中のものが残っていると、ロード完了コールバックが
		// 破棄済みのマップへ触れる恐れがあるので、その分は待たずとも
		// ここで一括で破棄する(シャットダウン時なので新規リクエストは来ない)。
		std::lock_guard<std::mutex> _lock(m_mutex);

		// GPUプール(パーティクル本体/デッドリスト/カウンタ/エミッタの各バッファを保持)を破棄。
		// unique_ptr の破棄で各バッファの ComPtr が解放され、デバイス参照が落ちる。
		m_pools.clear();

		// 発生命令のバッファ(全プール共通の1本)。終了時(GPUの完了待ちの後)なので、遅延させずにその場で返す
		m_upEmitterBuffer.reset();
		m_emitBufferCapacity = 0;

		// CPU側データ
		m_emitRequests.clear();
		m_frameEmitData.clear();
		m_emitRanges.clear();
		m_loadingHandles.clear();
		m_readyHandles.clear();
		m_overflowWarned.clear();
		m_lastEmitTime.clear();
		m_elapsedTime = 0.0;

		// 座席データの解放
		if (m_upEmitterSlotPool) m_upEmitterSlotPool->Release();
	}
	void ParticleBufferManager::BeginFrame(float a_dt)
	{
		// 眠っているプールを見分けるための時刻を進める(更新シェーダーと同じフレーム時間)
		m_elapsedTime += static_cast<double>((std::max)(a_dt, 0.0f));

		// 前のフレームで送った命令だけを消す、準備中に積まれたものは使えるようになるまで持ち越す
		for (auto& [_handle, _emitDataVec] : m_emitRequests)
		{
			if (IsReady(_handle))_emitDataVec.clear();
		}

		// ロードが終わったものをこのフレームから使えるようにする : ここでしか m_readyHandles を判定しない
		std::lock_guard<std::mutex> _lock(m_mutex);
		for (const auto& [_handle, _pool] : m_pools)
		{
			// ロード中の配列になければ
			if (!m_loadingHandles.contains(_handle))
			{
				// 準備完了
				m_readyHandles.insert(_handle);
			}
		}

		// 返却待ちの席の残り時間を進め、粒が消えきったものを空きへ戻す
		if (m_upEmitterSlotPool) m_upEmitterSlotPool->BeginFrame(a_dt);
	}
	void ParticleBufferManager::FinishFrame(D3D12::GraphicsCommandList* a_pCmdList)
	{
		if (!a_pCmdList) return;

		for (auto& [_handle, _upPool] : m_pools)
		{
			if (!_upPool || !_upPool->IsArgsReady()) continue;

			// 引数(INDIRECT_ARGUMENT)と生存リスト(NON_PIXEL_SHADER_RESOURCE)を COMMON へ。
			// 次のフレームの頭は COMMON から始まる約束
			_upPool->RefDrawArgs().Barrier(a_pCmdList, D3D12_RESOURCE_STATE_COMMON);
			_upPool->RefAliveList().Barrier(a_pCmdList, D3D12_RESOURCE_STATE_COMMON);
			_upPool->SetArgsReady(false);
		}
	}
	void ParticleBufferManager::RequestEmit(const Handle<Resource::ParticlesAsset>&a_handle, const EmitterData & a_emitterData)
	{
		if (!a_handle.IsValid()) return;

		// まだプールがなければ作らせる
		if (!m_pools.contains(a_handle))
		{
			CreateParticleDataAsync(a_handle);
			if (!m_pools.contains(a_handle)) return;
		}

		auto& _requests = m_emitRequests[a_handle];

		// 準備中はフレームをまたいでたまるので頭打ちにする(使えるようになった瞬間に塊で出ないように)
		if (!IsReady(a_handle) && _requests.size() >= EMIT_PENDING_REQUEST_MAX) return;

		// 発生を予約
		_requests.push_back(a_emitterData);
	}

	const std::unordered_map<Handle<Resource::ParticlesAsset>, std::unique_ptr<GPUParticlePool>>& ParticleBufferManager::GetPoolMap() const
	{
		return m_pools;
	}
	void ParticleBufferManager::UploadEmitData(D3D12::GraphicsCommandList* a_pCmdList, UINT a_frameIndex)
	{
		// 発生源の席の行列を送る。
		// 描画とシミュレーションはこのフレームの行列を読むので、命令より先に済ませておく
		if (m_upEmitterSlotPool && m_pGraphicsEngine)
		{
			m_upEmitterSlotPool->Upload(
				m_pGraphicsEngine->RefRenderDevice()->RefDevice(),
				m_pHeapManager,
				a_pCmdList,
				a_frameIndex);
		}

		//----------------------------------------------------------------------
		// 全プールの命令を1本につなげる
		//
		// プールごとに「共通の1本の中のどこからどこまでか」を覚えておき、
		// 発生の Dispatch はそこだけを読む。準備中のプールの命令はつなげずに持ち越す
		//----------------------------------------------------------------------
		m_frameEmitData.clear();
		m_emitRanges.clear();

		for (auto& [_handle, _emitDataVec] : m_emitRequests)
		{
			// リクエストがない、またはまだGPUバッファが生成中ならスキップ
			if (_emitDataVec.empty() || !IsReady(_handle))
			{
				continue;
			}

			// 全体の上限を超えるぶんは、このフレームでは出さない
			const size_t _room = static_cast<size_t>(EMIT_BUFFER_MAX_CAPACITY) - m_frameEmitData.size();
			const size_t _num = (std::min)(_emitDataVec.size(), _room);

			//----------------------------------------------------------------------
			// あふれたことを知らせる
			//
			// 黙って捨てると、欠けているのか元からそういう絵なのかが見分けられない。
			// 毎フレーム出すとログが埋まるので、アセットごとに1回だけ
			//----------------------------------------------------------------------
			if (_num < _emitDataVec.size() && !m_overflowWarned.contains(_handle))
			{
				m_overflowWarned.insert(_handle);

				const auto* _pResourceManager = m_pGraphicsEngine ? m_pGraphicsEngine->RefResourceManager() : nullptr;
				const auto* _pParticle = _pResourceManager ? _pResourceManager->Get(_handle) : nullptr;

				ENGINE_WARNING(
					"[Particle] 発生命令が1フレームの上限(全体で %u 件)を超えました : %s。あふれた分はこのフレームでは出ません",
					EMIT_BUFFER_MAX_CAPACITY,
					_pParticle ? _pParticle->GetName().c_str() : "(不明)");
			}

			if (_num == 0) continue;

			EmitRange _range = {};
			_range.offset = static_cast<uint32_t>(m_frameEmitData.size());
			_range.count = static_cast<uint32_t>(_num);
			m_emitRanges[_handle] = _range;

			m_frameEmitData.insert(m_frameEmitData.end(), _emitDataVec.begin(), _emitDataVec.begin() + _num);

			// このフレームに粒が出る : ここから最大寿命ぶんは起こしておく
			m_lastEmitTime[_handle] = m_elapsedTime;
		}

		if (m_frameEmitData.empty() || !m_pGraphicsEngine) return;

		//----------------------------------------------------------------------
		// 足りなければ作り直す
		//
		// 2のべき乗で伸ばすので、作り直しはめったに起きない。
		// 中身は毎フレーム全部送り直すので、古いバッファから写す必要はない。
		// 古いバッファは前のフレームの発生がまだ読んでいるかもしれないので、
		// GPU が使い終わるまで遅延させて捨てる(EmitterSlotPool と同じ)
		//----------------------------------------------------------------------
		const uint32_t _need = static_cast<uint32_t>(m_frameEmitData.size());
		if (!m_upEmitterBuffer || m_emitBufferCapacity < _need)
		{
			uint32_t _newCapacity = (std::max)(m_emitBufferCapacity, EMIT_BUFFER_MIN_CAPACITY);
			while (_newCapacity < _need)
			{
				_newCapacity *= 2;
			}
			_newCapacity = (std::min)(_newCapacity, EMIT_BUFFER_MAX_CAPACITY);

			if (m_upEmitterBuffer)
			{
				std::shared_ptr<D3D12::StaticStructuredBuffer<EmitterData>> _spOld(std::move(m_upEmitterBuffer));
				MainEngine::Instance().RegisterDeferredResource([_spOld]() {});
			}

			m_upEmitterBuffer = std::make_unique<D3D12::StaticStructuredBuffer<EmitterData>>();
			m_upEmitterBuffer->Create(
				m_pGraphicsEngine->RefRenderDevice()->RefDevice(),
				m_pHeapManager,
				a_pCmdList,
				_newCapacity,
				nullptr);
			m_emitBufferCapacity = _newCapacity;
		}

		// 毎フレーム書き換えるので、フレームごとの区画を経由してGPUへ送る
		m_upEmitterBuffer->UploadFrame(
			a_pCmdList,
			m_frameEmitData.data(),
			sizeof(EmitterData) * m_frameEmitData.size(),
			a_frameIndex);
	}

	bool ParticleBufferManager::IsAwake(const Handle<Resource::ParticlesAsset>& a_handle) const
	{
		// 一度も出していなければ、生きている粒は無い
		auto _it = m_lastEmitTime.find(a_handle);
		if (_it == m_lastEmitTime.end()) return false;

		// 寿命が分からないときは起こしておく(止めて粒が固まるより、回して無駄になる方が害が小さい)
		const auto* _pResourceManager = m_pGraphicsEngine ? m_pGraphicsEngine->RefResourceManager() : nullptr;
		const auto* _pParticle = _pResourceManager ? _pResourceManager->Get(a_handle) : nullptr;
		if (!_pParticle) return true;

		// 粒の寿命は [LifeTimeMin, LifeTimeMax] の乱数なので、Max が経てば全部消えている。
		// 寿命の下限は発生シェーダーが 0.0001 秒に丸めるので、0 でも余裕のぶんは起きている
		const double _lifeMax = static_cast<double>((std::max)(_pParticle->GetLifeTimeMax(), _pParticle->GetLifeTimeMin()));
		return (m_elapsedTime - _it->second) <= (_lifeMax + PARTICLE_POOL_SLEEP_MARGIN_SECONDS);
	}

	double ParticleBufferManager::GetSecondsSinceLastEmit(const Handle<Resource::ParticlesAsset>& a_handle) const
	{
		auto _it = m_lastEmitTime.find(a_handle);
		if (_it == m_lastEmitTime.end()) return -1.0;
		return m_elapsedTime - _it->second;
	}
	std::span <const EmitterData> ParticleBufferManager::GetRequests(const Handle<Resource::ParticlesAsset>& a_assetHandle) const
	{
		auto _it = m_emitRequests.find(a_assetHandle);
		if (_it != m_emitRequests.end())
		{
			return _it->second;
		}
		return {};
	}
	const D3D12::StaticStructuredBuffer<EmitterData>* ParticleBufferManager::GetEmitterBuffer() const
	{
		return m_upEmitterBuffer.get();
	}
	ParticleBufferManager::EmitRange ParticleBufferManager::GetEmitRange(const Handle<Resource::ParticlesAsset>& a_handle) const
	{
		auto _it = m_emitRanges.find(a_handle);
		if (_it != m_emitRanges.end())
		{
			return _it->second;
		}
		return {};
	}
	void ParticleBufferManager::CreateParticleDataAsync(const Handle<Resource::ParticlesAsset>& a_handle)
	{
		// デバイス取得
		if (!m_pGraphicsEngine) return;
		auto* _pDevice = m_pGraphicsEngine->RefRenderDevice()->RefDevice();

		//----------------------------------------------------------------------
		// アセットが読めていなければ、まだ作らない
		//
		// 容量はアセットから引くので、読めていないと作りようがない。
		// ここで弾かずに進めると「中身の無いプール」が m_pools に残り、
		// BeginFrame がそれを準備完了にして、更新の Dispatch が
		// 作られていないバッファの番号で走ってしまう。
		// 弾いておけば、次の RequestEmit(または Warmup)で作り直しに来る
		//----------------------------------------------------------------------
		if (!a_handle.IsValid()) return;
		const auto* _pResourceManager = m_pGraphicsEngine->RefResourceManager();
		if (!_pResourceManager || !_pResourceManager->Get(a_handle)) return;

		// メインスレッド側でマップ作成
		{
			std::lock_guard<std::mutex> _lock(m_mutex);

			// すでに登録済み、ロード中ならリターン
			if (m_pools.find(a_handle) != m_pools.end() || m_loadingHandles.find(a_handle) != m_loadingHandles.end())
			{
				return;
			}

			// ロード中リストに追加して、空のコンテナを用意
			m_loadingHandles.insert(a_handle);
			m_pools[a_handle] = std::make_unique<GPUParticlePool>();
			m_emitRequests[a_handle] = std::vector<EmitterData>();
		}

		// コンピュート用の計算を非同期マネージャーへ流す
		// ※ 記録(1つ目のラムダ)は ExecuteAsyncCopy の中でその場で走る。非同期なのは GPU 側のコピーだけ
		bool _isCreated = false;
		m_pGraphicsEngine->RefRenderDevice()->ExecuteAsyncCopy(
			// ロード処理
			[this,_pDevice,a_handle,&_isCreated](D3D12::GraphicsCommandList* a_pCmdList)
			{
				// 発生命令のバッファは全プール共通の1本(UploadEmitData が持つ)なので、ここではプール本体だけ作る
				if (!m_pools[a_handle]->Init(_pDevice, m_pHeapManager, a_pCmdList, a_handle, *m_pGraphicsEngine->RefResourceManager()))
				{
					return;
				}
				_isCreated = true;
			},
			// コールバック処理
			[this,a_handle]()
			{
				std::lock_guard<std::mutex> _lock(m_mutex);
				m_loadingHandles.erase(a_handle);

				ENGINE_LOG("パーティクルGPUデータ作成完了");
			}
		);

		// 作れなかったら登録を取り消す。
		// 残すと BeginFrame が中身の無いプールを準備完了にしてしまう。
		// ロード中の印はコールバックが外すので、ここでは触らない
		// (外すまでの間に来た作成依頼は「ロード中」で弾かれ、次のフレーム以降に作り直される)
		if (!_isCreated)
		{
			std::lock_guard<std::mutex> _lock(m_mutex);
			m_pools.erase(a_handle);
			m_emitRequests.erase(a_handle);
		}
	}
}