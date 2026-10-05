#include "ParticleBufferManager.h"

#include "../../Resource/Manager/AssetDatabase/AssetDatabase.h"
#include "../GraphicsEngine.h"
#include "../../Resource/Manager/ResourceManager/ResourceManager.h"

namespace Engine::Particle
{
	void Engine::Particle::ParticleBufferManager::Init(
		Graphics::GraphicsEngine* a_pGraphicsEngine,
		D3D12::DescriptorHeapManager* a_pHeapManager,
		D3D12::GraphicsCommandList* a_pCmdList
	)
	{
		// ビューの置き場と転送の依頼先を控える : プールは非同期に作られるので、そこまで持ち回る
		m_pHeapManager = a_pHeapManager;
		m_pGraphicsEngine = a_pGraphicsEngine;
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

		// いまフレームのエミット命令バッファを破棄
		m_emitBuffer.clear();

		// CPU側データ
		m_emitRequests.clear();
		m_emitterSlots.clear();
		m_loadingHandles.clear();
		m_readyHandles.clear();
		m_overflowWarned.clear();
	}
	void ParticleBufferManager::BeginFrame()
	{
		// 席の使用状況を測るためのフレーム番号。
		// 「しばらく使われていない席」を見分けるのに使う
		++m_frameCount;

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

		// 準備中はフレームをまたいでたまるので命令バッファの長さで頭打ちにする
		if (!IsReady(a_handle) && _requests.size() >= EMIT_REQUEST_MAX) return;

		// 発生を予約
		_requests.push_back(a_emitterData);
	}
	//======================================================================================
	// 発生源の席
	//======================================================================================
	uint32_t ParticleBufferManager::AcquireEmitterSlot(
		const Handle<Resource::ParticlesAsset>& a_handle,
		uint64_t a_ownerKey,
		const Math::Matrix& a_ownerWorld)
	{
		EmitterSlotTable& _table = m_emitterSlots[a_handle];

		// 席 0 は単位行列で予約。ワールド空間の粒がここを指す
		if (_table.matrices.empty())
		{
			_table.matrices.push_back(Math::Matrix{});
			_table.slotOwners.push_back(0);
			_table.slotUsedFrame.push_back(0);
		}

		const Math::Matrix _mat = StripScale(a_ownerWorld);

		// すでに席を持っているなら行列だけ更新する
		auto _it = _table.slotMap.find(a_ownerKey);
		if (_it != _table.slotMap.end())
		{
			_table.matrices[_it->second]     = _mat;
			_table.slotUsedFrame[_it->second] = m_frameCount;
			return _it->second;
		}

		//----------------------------------------------------------------------
		// 空いている席があれば、そこへ座る
		//----------------------------------------------------------------------
		if (_table.matrices.size() < PARTICLE_EMITTER_MAX)
		{
			const uint32_t _slot = static_cast<uint32_t>(_table.matrices.size());
			_table.matrices.push_back(_mat);
			_table.slotOwners.push_back(a_ownerKey);
			_table.slotUsedFrame.push_back(m_frameCount);
			_table.slotMap.emplace(a_ownerKey, _slot);

			return _slot;
		}

		//----------------------------------------------------------------------
		// 席が尽きた : しばらく使われていない席を回す
		//
		// 鍵はエンティティなので、シーンを読み直すたびに作り直されるもの
		// (ブースターなど)は毎回ちがう鍵で席を取る。返す仕組みが無いと
		// 数回の読み直しで席が尽き、そこから先はワールド空間で出てしまう。
		//
		// 回してよいのは EMITTER_SLOT_KEEP_FRAMES のあいだ一度も使われていない席だけ。
		// 出し終わった粒が消えるまでの猶予をここで取っているので、
		// まだ生きている粒の行列を奪うことにはならない。
		//----------------------------------------------------------------------
		uint32_t _oldestSlot = 0;
		uint64_t _oldestFrame = m_frameCount;

		for (uint32_t _i = 1; _i < static_cast<uint32_t>(_table.slotUsedFrame.size()); ++_i)
		{
			if (_table.slotUsedFrame[_i] < _oldestFrame)
			{
				_oldestFrame = _table.slotUsedFrame[_i];
				_oldestSlot  = _i;
			}
		}

		const bool _isStale =
			(_oldestSlot != 0) &&
			((m_frameCount - _oldestFrame) >= EMITTER_SLOT_KEEP_FRAMES);

		if (!_isStale)
		{
			// 全部が現役 : 奪うと生きている粒が化けるので、ワールド空間で出す
			ENGINE_WARNING(
				"[Particle] 発生源の席が足りません(上限 %d)。ワールド空間で出します",
				static_cast<int>(PARTICLE_EMITTER_MAX));
			return 0;
		}

		// 前の持ち主を忘れて、席を引き継ぐ
		const uint64_t _prevOwner = _table.slotOwners[_oldestSlot];
		if (_prevOwner != 0)
		{
			_table.slotMap.erase(_prevOwner);
		}

		_table.matrices[_oldestSlot]     = _mat;
		_table.slotOwners[_oldestSlot]   = a_ownerKey;
		_table.slotUsedFrame[_oldestSlot] = m_frameCount;
		_table.slotMap.emplace(a_ownerKey, _oldestSlot);

		return _oldestSlot;
	}

	std::span<const Math::Matrix> ParticleBufferManager::GetEmitterMatrices(
		const Handle<Resource::ParticlesAsset>& a_handle) const
	{
		auto _it = m_emitterSlots.find(a_handle);
		if (_it == m_emitterSlots.end()) return {};

		return std::span<const Math::Matrix>(_it->second.matrices);
	}

	const std::unordered_map<Handle<Resource::ParticlesAsset>, std::unique_ptr<GPUParticlePool>>& ParticleBufferManager::GetPoolMap() const
	{
		return m_pools;
	}
	void ParticleBufferManager::UploadEmitData(D3D12::GraphicsCommandList* a_pCmdList, UINT a_frameIndex)
	{
		for (auto& [_handle, _emitDataVec] : m_emitRequests)
		{
			// リクエストがない、またはまだGPUバッファが生成中ならスキップ
			if (_emitDataVec.empty() || !IsReady(_handle))
			{
				continue;
			}

			auto _it = m_emitBuffer.find(_handle);
			if (_it != m_emitBuffer.end())
			{
				// バッファは固定長。要素数を超えて書くとマップ領域を踏み越えるので切り詰める。
				// (パス側も同じ数で requestCount を丸めるので、あふれた命令はこのフレームでは捨てる)
				const size_t _uploadNum = (std::min)(_emitDataVec.size(), _it->second.GetElementNum());

				//----------------------------------------------------------------------
				// あふれたことを知らせる
				//
				// 黙って捨てると「群れの着地で一部だけ砂煙が出ない」のように、
				// 欠けているのか元からそういう絵なのかが見分けられない。
				// 毎フレーム出すとログが埋まるので、アセットごとに1回だけ
				//----------------------------------------------------------------------
				if (_emitDataVec.size() > _uploadNum && !m_overflowWarned.contains(_handle))
				{
					m_overflowWarned.insert(_handle);

					const auto* _pResourceManager = m_pGraphicsEngine ? m_pGraphicsEngine->RefResourceManager() : nullptr;
					const auto* _pParticle = _pResourceManager ? _pResourceManager->Get(_handle) : nullptr;

					ENGINE_WARNING(
						"[Particle] 発生命令があふれました : %s (%d 件 / 上限 %d 件)。あふれた分はこのフレームでは出ません",
						_pParticle ? _pParticle->GetName().c_str() : "(不明)",
						static_cast<int>(_emitDataVec.size()),
						static_cast<int>(_uploadNum));
				}

				// 毎フレーム書き換えるので、フレームごとの区画を経由してGPUへ送る
				_it->second.UploadFrame(a_pCmdList, _emitDataVec.data(), sizeof(EmitterData) * _uploadNum, a_frameIndex);
			}
		}
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
	const D3D12::StaticStructuredBuffer<EmitterData>* ParticleBufferManager::GetEmitBuffer(const Handle<Resource::ParticlesAsset>& a_handle) const
	{
		auto _it = m_emitBuffer.find(a_handle);
		if (_it != m_emitBuffer.end())
		{
			return &_it->second;
		}
		return nullptr;
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
				if (!m_pools[a_handle]->Init(_pDevice, m_pHeapManager, a_pCmdList, a_handle, *m_pGraphicsEngine->RefResourceManager()))
				{
					return;
				}
				m_emitBuffer[a_handle].Create(_pDevice, m_pHeapManager, a_pCmdList, static_cast<UINT>(EMIT_REQUEST_MAX), nullptr);
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
			m_emitBuffer.erase(a_handle);
		}
	}
	void ParticleBufferManager::RefreshEmitterSlot(
		const Handle<Resource::ParticlesAsset>& a_handle,
		uint64_t a_ownerKey,
		const Math::Matrix& a_ownerWorld
	)
	{
		auto _tableIt = m_emitterSlots.find(a_handle);
		if (_tableIt == m_emitterSlots.end()) return;

		EmitterSlotTable& _table = _tableIt->second;

		// 席を持っていなければ何もしない(取るのは AcquireEmitterSlot だけ)。
		// 回された席は前の持ち主の鍵ごと slotMap から消えているので、
		// 席を奪われた側がここで新しい持ち主の行列を書き潰すことはない
		auto _it = _table.slotMap.find(a_ownerKey);
		if (_it == _table.slotMap.end()) return;

		// 使用フレームは進めない。
		// 進めると「出していないが生きている」発生源(待機中のブースターなど)が
		// 席を握り続け、出し終わった席を回すという今の回収の決まりが崩れる
		_table.matrices[_it->second] = StripScale(a_ownerWorld);
	}

	//----------------------------------------------------------------------
	// 拡縮を落として、位置と回転だけを残す
	//
	// 取り付け側のスケール(ブースターは 0.1 倍など)を残したまま戻すと、
	// ローカルで進めた飛距離までそのスケールで縮んでしまう。
	// 粒は最初からワールドの尺で飛ばしたいので、軸の長さを 1 に揃える
	//----------------------------------------------------------------------
	Math::Matrix ParticleBufferManager::StripScale(const Math::Matrix& a_world)
	{
		Math::Matrix _mat = a_world;

		Math::Vector3 _axisX(_mat._11, _mat._12, _mat._13);
		Math::Vector3 _axisY(_mat._21, _mat._22, _mat._23);
		Math::Vector3 _axisZ(_mat._31, _mat._32, _mat._33);

		if (_axisX.LengthSquared() > 1e-12f) _axisX.Normalize();
		if (_axisY.LengthSquared() > 1e-12f) _axisY.Normalize();
		if (_axisZ.LengthSquared() > 1e-12f) _axisZ.Normalize();

		_mat._11 = _axisX.x; _mat._12 = _axisX.y; _mat._13 = _axisX.z;
		_mat._21 = _axisY.x; _mat._22 = _axisY.y; _mat._23 = _axisY.z;
		_mat._31 = _axisZ.x; _mat._32 = _axisZ.y; _mat._33 = _axisZ.z;

		return _mat;
	}
}