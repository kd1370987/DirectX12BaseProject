#include "ParticleBufferManager.h"

#include "../Resource/Manager/AssetDatabase/AssetDatabase.h"
#include "../Resource/Loader/Particles/ParticlesLoader.h"

namespace Engine::Particle
{
	void Engine::Particle::ParticleBufferManager::Init(
		D3D12::Device* a_pDevice,
		D3D12::GraphicsCommandList* a_pCmdList
	)
	{
		// パーティクルのデータとバッファ自体は軽いのでいったん初期化時に全生成
		auto _propVec = Resource::AssetDatabase::Instance().GetTypeMetaVec("ParticlesAsset");
		for (const auto& _prop : _propVec)
		{
			auto _handle = Resource::ParticlesAssetLoader::Load(_prop.guid);

			// GPUプールの作成
			m_pools[_handle] = std::make_unique<GPUParticlePool>();
			m_pools[_handle]->Init(a_pDevice, a_pCmdList, _handle);

			// エミットデータの空生成
			m_emitRequests[_handle] = std::vector<EmitterData>();

			// 構造体バッファの作成
			m_emitBuffer[_handle].Create(a_pDevice, a_pCmdList, 100, nullptr);
		}
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
		m_emitRequests[a_handle].push_back(a_emitterData);
	}
	const std::unordered_map<Handle<Resource::ParticlesAsset>, std::unique_ptr<GPUParticlePool>>& ParticleBufferManager::GetPoolMap() const
	{
		return m_pools;
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
}