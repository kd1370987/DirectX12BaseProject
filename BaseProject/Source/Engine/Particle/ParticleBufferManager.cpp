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
			ENGINE_LOG("パーティクルが登録されました : %d", a_emitterData.emitCount);
		}
		else
		{
			// 新規作成
			CreateParticleDataAsync(a_handle);
		}
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
				// バッファにデータを流し込む
				_it->second.UpdateData(_emitDataVec.data(), sizeof(EmitterData) * _emitDataVec.size());
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