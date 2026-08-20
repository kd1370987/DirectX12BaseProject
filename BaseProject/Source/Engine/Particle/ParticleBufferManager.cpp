#include "ParticleBufferManager.h"

#include "../Resource/Manager/AssetDatabase/AssetDatabase.h"

#include "../D3D12/D3D12Wrapper/D3D12Wrapper.h"

namespace Engine::Particle
{
	void Engine::Particle::ParticleBufferManager::Init(
		D3D12::Device* a_pDevice,
		D3D12::GraphicsCommandList* a_pCmdList
	)
	{
		// パーティクルのデータとバッファ自体は軽いのでいったん初期化時に全生成
		//auto _propVec = Resource::AssetDatabase::Instance().GetTypeMetaVec("ParticlesAsset");
		//for (const auto& _prop : _propVec)
		//{
		//	auto _handle = Resource::ParticlesAssetLoader::Load(_prop.guid);

		//	// GPUプールの作成
		//	m_pools[_handle] = std::make_unique<GPUParticlePool>();
		//	m_pools[_handle]->Init(a_pDevice, a_pCmdList, _handle);

		//	// エミットデータの空生成
		//	m_emitRequests[_handle] = std::vector<EmitterData>();

		//	// 構造体バッファの作成
		//	m_emitBuffer[_handle].Create(a_pDevice, a_pCmdList, 100, nullptr);
		//}
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
	}
	void ParticleBufferManager::BeginFrame()
	{
		// リクエストのクリア
		for (auto& [_handle, _emitDataVec] : m_emitRequests)
		{
			_emitDataVec.clear();
		}
	}
	void ParticleBufferManager::RequestEmit(const Handle<Resource::ParticlesAsset>&a_handle, const EmitterData & a_emitterData)
	{
		if (a_handle.id == 0) return;
		if (a_handle == Handle<Resource::ParticlesAsset>()) { return; }

		{
			std::lock_guard<std::mutex> _lock(m_mutex);
			if (m_loadingHandles.find(a_handle) != m_loadingHandles.end())
			{
				// まだバッファが出来上がっていないのでリクエストを破棄
				return;
			}
		}

		auto _it = m_emitRequests.find(a_handle);
		if (_it != m_emitRequests.end())
		{
			// すでに読み込まれたことのあるパーティクルなら
			_it->second.push_back(a_emitterData);
		}
		else
		{
			// 新規作成
			CreateParticleDataAsync(a_handle);
		}
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
		}

		//----------------------------------------------------------------------
		// 拡縮を落として、位置と回転だけを覚える
		//
		// 取り付け側のスケール(ブースターは 0.1 倍など)を残したまま戻すと、
		// ローカルで進めた飛距離までそのスケールで縮んでしまう。
		// 粒は最初からワールドの尺で飛ばしたいので、軸の長さを 1 に揃える
		//----------------------------------------------------------------------
		DXSM::Matrix _mat = a_ownerWorld;
		{
			DXSM::Vector3 _axisX(_mat._11, _mat._12, _mat._13);
			DXSM::Vector3 _axisY(_mat._21, _mat._22, _mat._23);
			DXSM::Vector3 _axisZ(_mat._31, _mat._32, _mat._33);

			if (_axisX.LengthSquared() > 1e-12f) _axisX.Normalize();
			if (_axisY.LengthSquared() > 1e-12f) _axisY.Normalize();
			if (_axisZ.LengthSquared() > 1e-12f) _axisZ.Normalize();

			_mat._11 = _axisX.x; _mat._12 = _axisX.y; _mat._13 = _axisX.z;
			_mat._21 = _axisY.x; _mat._22 = _axisY.y; _mat._23 = _axisY.z;
			_mat._31 = _axisZ.x; _mat._32 = _axisZ.y; _mat._33 = _axisZ.z;
		}

		// すでに席を持っているなら行列だけ更新する
		auto _it = _table.slotMap.find(a_ownerKey);
		if (_it != _table.slotMap.end())
		{
			_table.matrices[_it->second] = _mat;
			return _it->second;
		}

		// 席が尽きたらワールド空間として出す。
		// 席を奪い合うと、まだ生きている粒の行列が別の発生源のものへ化ける
		if (_table.matrices.size() >= PARTICLE_EMITTER_MAX)
		{
			ENGINE_WARNING(
				"[Particle] 発生源の席が足りません(上限 %d)。ワールド空間で出します",
				static_cast<int>(PARTICLE_EMITTER_MAX));
			return 0;
		}

		const uint32_t _slot = static_cast<uint32_t>(_table.matrices.size());
		_table.matrices.push_back(_mat);
		_table.slotMap.emplace(a_ownerKey, _slot);

		return _slot;
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
	void ParticleBufferManager::UploadEmitData(D3D12::GraphicsCommandList* a_pCmdList)
	{
		for (auto& [_handle, _emitDataVec] : m_emitRequests)
		{
			// リクエストがない、またはまだGPUバッファが生成中ならスキップ
			if (_emitDataVec.empty() || IsLoading(_handle))
			{
				continue;
			}

			auto _it = m_emitBuffer.find(_handle);
			if (_it != m_emitBuffer.end())
			{
				// バッファは固定長。要素数を超えて書くとマップ領域を踏み越えるので切り詰める。
				// (パス側も同じ数で requestCount を丸めるので、あふれた命令はこのフレームでは捨てる)
				const size_t _uploadNum = (std::min)(_emitDataVec.size(), _it->second.GetElementNum());

				// バッファにデータを流し込む
				_it->second.UpdateData(_emitDataVec.data(), sizeof(EmitterData) * _uploadNum);
				// GPUへの転送コマンドを積む
				_it->second.Update(a_pCmdList);
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
		auto* _pDevice = D3D12::D3D12Wrapper::Instance().GetDevice();

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
		D3D12::D3D12Wrapper::Instance().ExecuteAsyncCopy(
			// ロード処理
			[this,_pDevice,a_handle](D3D12::GraphicsCommandList* a_pCmdList)
			{
				m_pools[a_handle]->Init(_pDevice, a_pCmdList, a_handle);
				m_emitBuffer[a_handle].Create(_pDevice, a_pCmdList, 100, nullptr);
			},
			// コールバック処理
			[this,a_handle]()
			{
				std::lock_guard<std::mutex> _lock(m_mutex);
				m_loadingHandles.erase(a_handle);

				ENGINE_LOG("パーティクルGPUデータ作成完了");
			}
		);
	}
	bool ParticleBufferManager::IsLoading(const Handle<Resource::ParticlesAsset>& a_handle)
	{
		// 別スレッドが書き換えている可能性があるのでロックをかける
		std::lock_guard<std::mutex> _lock(m_mutex);
		return m_loadingHandles.find(a_handle) != m_loadingHandles.end();
	}
	bool ParticleBufferManager::IsLoaded(const Handle<Resource::ParticlesAsset>& a_handle)
	{
		std::lock_guard<std::mutex> _lock(m_mutex);
		return m_loadingHandles.find(a_handle) == m_loadingHandles.end();
	}
}