#include "ParticleBufferManager.h"

#include "../../Resource/Manager/AssetDatabase/AssetDatabase.h"
#include "../GraphicsEngine.h"
#include "../../Resource/Manager/ResourceManager/ResourceManager.h"

#include "GPU/EmitterSlotPool/EmitterSlotPool.h"

#include "Engine/MainEngine.h"	// GPU が使い終わってからの解放(RegisterDeferredResource)

namespace
{
	//======================================================================================
	// GPU が使い終わってから Release() を呼んで捨てる
	//
	// ・前のフレームのコマンドがまだ読んでいるかもしれないので、その場では捨てない。
	//   遅延解放のキューは、そのフレームの GPU 完了を待ってから流れる
	//   (終了時は MainEngine が WaitForGPUIdle の後にまとめて流す)。
	// ・バッファは壊すだけではディスクリプタヒープの席が返らない(返すのは Release())。
	//   Release() を呼ばずに shared_ptr を手放すだけだと、そのたびに席が漏れる
	//======================================================================================
	template<typename T>
	void ReleaseAfterGPU(std::unique_ptr<T>& a_upTarget)
	{
		if (!a_upTarget) return;

		std::shared_ptr<T> _spTarget(std::move(a_upTarget));
		Engine::MainEngine::Instance().RegisterDeferredResource([_spTarget]() { _spTarget->Release(); });
	}
}

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

		//----------------------------------------------------------------------
		// GPU のバッファを返す
		//
		// 終了時は GPU の完了待ちより前に呼ばれる(MainEngine::Release → GraphicsEngine::Release)。
		// 最後のフレームがまだ読んでいるかもしれないので、完了待ちの後に流れる遅延解放へ回す。
		// 遅延解放は DescriptorHeapManager の解放より前に流れるので、ディスクリプタも返せる
		//----------------------------------------------------------------------
		for (auto& [_handle, _upPool] : m_pools)
		{
			ReleaseAfterGPU(_upPool);
		}
		m_pools.clear();

		// 発生命令のバッファ(全プール共通の1本)
		ReleaseAfterGPU(m_upEmitterBuffer);
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

		// 使われなくなったプールを捨てる。
		// 下の準備完了の判定(m_mutex を関数の終わりまで持つ)より前に済ませておく
		// (DestroyPool も m_mutex を取るので、ロックの中で呼ぶとデッドロックする)
		ReleaseUnusedPools();

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
	//======================================================================================
	// 使われなくなったプールを捨てる
	//
	// 取り残されたプール : シーンの切れ目で ResourceManager::SweepUnusedAll が
	//   参照の切れた ParticlesAsset を捨てると、そのハンドルは無効になる。
	//   プールはそのハンドルをキーにしたまま残り、次に読み直したアセットは別のハンドルで
	//   別のプールを作るので、シーンを行き来するたびにプールが増えていく。
	//   アセットが引けないプールは描くこともできないので捨てる。
	//
	// ロード中のプールは捨てない : コピーキューがまだバッファへ書いているかもしれない
	//======================================================================================
	void ParticleBufferManager::ReleaseUnusedPools()
	{
		const auto* _pResourceManager = m_pGraphicsEngine ? m_pGraphicsEngine->RefResourceManager() : nullptr;
		if (!_pResourceManager) return;

		// 回しながら消すとイテレーターが壊れるので、先に集める
		std::vector<Handle<Resource::ParticlesAsset>> _orphans;
		{
			std::lock_guard<std::mutex> _lock(m_mutex);
			for (const auto& [_handle, _upPool] : m_pools)
			{
				if (m_loadingHandles.contains(_handle)) continue;
				if (!_pResourceManager->Get(_handle))
				{
					_orphans.push_back(_handle);
				}
			}
		}

		for (const auto& _handle : _orphans)
		{
			DestroyPool(_handle, "アセットが破棄された");
		}
	}

	void ParticleBufferManager::DestroyPool(const Handle<Resource::ParticlesAsset>& a_handle, const char* a_reason)
	{
		// 登録から外す(作成の登録と同じく m_mutex の中で)
		std::unique_ptr<GPUParticlePool> _upPool;
		{
			std::lock_guard<std::mutex> _lock(m_mutex);
			auto _it = m_pools.find(a_handle);
			if (_it == m_pools.end()) return;

			_upPool = std::move(_it->second);
			m_pools.erase(_it);
		}

		// このプールに紐づく CPU 側の記録も消す。
		// 次に必要になれば RequestEmit / Warmup が作り直す(命令は作り直しのあいだ持ち越される)
		m_emitRequests.erase(a_handle);
		m_emitRanges.erase(a_handle);
		m_readyHandles.erase(a_handle);
		m_overflowWarned.erase(a_handle);
		m_lastEmitTime.erase(a_handle);

		// GPU が使い終わってからバッファを返す。
		// BeginFrame から呼ぶので、引数の印(IsArgsReady)は前のフレームの FinishFrame で下りていて、
		// バッファの状態は COMMON に戻っている
		ReleaseAfterGPU(_upPool);

		ENGINE_LOG("[Particle] プールを解放しました : id=%u (%s)", a_handle.id, a_reason ? a_reason : "");
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

			m_frameEmitData.insert(m_frameEmitData.end(), _emitDataVec.begin(), _emitDataVec.begin() + _num);

			//----------------------------------------------------------------------
			// 粒ごとの開始位置を入れる(発生シェーダーは 1スレッド = 1粒)
			//
			// 各命令に「このプールの中で何粒目から始まるか」を持たせ、合計を数える。
			// シェーダーはスレッド番号からこれを二分探索して「どの命令の何個目か」を引く。
			// 合計はプールの容量で頭打ちにする。容量を超えた粒は空き番号が無くて出せないので、
			// スレッドを立てるだけ無駄になる(後ろの命令の粒から出なくなる)
			//----------------------------------------------------------------------
			uint64_t _emitTotal = 0;
			for (size_t _r = _range.offset; _r < m_frameEmitData.size(); ++_r)
			{
				m_frameEmitData[_r].emitStart = static_cast<UINT>((std::min)(_emitTotal, static_cast<uint64_t>(UINT32_MAX)));
				_emitTotal += m_frameEmitData[_r].emitCount;
			}

			const auto _poolIt = m_pools.find(_handle);
			const uint64_t _capacity = (_poolIt != m_pools.end() && _poolIt->second) ? _poolIt->second->GetMaxCapacity() : 0;
			_range.emitTotal = static_cast<uint32_t>((std::min)(_emitTotal, _capacity));

			m_emitRanges[_handle] = _range;

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

			// 壊すだけではディスクリプタ(SRV)が返らないので、Release() を呼んでから手放す
			ReleaseAfterGPU(m_upEmitterBuffer);

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

		// アセットが引けなければ眠っている扱い。
		// 描画側もアセットを引けずに飛ばすので、起こしておいても更新が空回りするだけになる
		// (アセットが捨てられて取り残されたプールは、次の BeginFrame で ReleaseUnusedPools が捨てる)
		const auto* _pResourceManager = m_pGraphicsEngine ? m_pGraphicsEngine->RefResourceManager() : nullptr;
		const auto* _pParticle = _pResourceManager ? _pResourceManager->Get(a_handle) : nullptr;
		if (!_pParticle) return false;

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