#include "GPUParticlePool.h"

#include "../../../Resource/Manager/ResourceManager/ResourceManager.h"

namespace Engine::Particle
{
	void Engine::Particle::GPUParticlePool::Init(ID3D12Device* a_pDevice, ID3D12GraphicsCommandList* a_pCmdList,Engine::Handle<Resource::ParticlesAsset> a_particleHandle)
	{
		auto* _pParticleAsset = Resource::ResourceManager::Instance().Get(a_particleHandle);
		if (!_pParticleAsset) return;

		// パーティクルデータの確保
		m_maxCapacity = _pParticleAsset->GetCapacity();
		m_assetHandle = a_particleHandle;

		// バッファの作成
		m_particlePool.Create(a_pDevice, m_maxCapacity);
		m_deadList.Create(a_pDevice,m_maxCapacity);
		m_counterBuffer.Create(a_pDevice,1);

		// バッファの初期化用データの作成
		std::vector<uint32_t> _initDeadList(m_maxCapacity);
		std::iota(_initDeadList.begin(),_initDeadList.end(),0); // すべての配列を0から連番で埋めてくれる
		uint32_t _initCounter = m_maxCapacity;

		// アップロードバッファを作成してコピーする
		D3D12::DynamicBuffer _deadListUpload = {};
		D3D12::DynamicBuffer _counterUpload = {};

		D3D12::DynamicBufferDesc _deadDesc = { m_maxCapacity, sizeof(uint32_t), D3D12_RESOURCE_FLAG_NONE };
		D3D12::DynamicBufferDesc _countDesc = { 1, sizeof(uint32_t), D3D12_RESOURCE_FLAG_NONE };

		_deadListUpload.Create(a_pDevice, _deadDesc);
		_counterUpload.Create(a_pDevice, _countDesc);

		// データを書き込む
		_deadListUpload.UpdateData(_initDeadList.data(), m_maxCapacity * sizeof(uint32_t));
		_counterUpload.UpdateData(&_initCounter, sizeof(uint32_t));

		// コピー前のリソースバリア (DEFAULTヒープを COPY_DEST にする)
		m_deadList.Barrier(a_pCmdList, D3D12_RESOURCE_STATE_COPY_DEST);
		m_counterBuffer.Barrier(a_pCmdList, D3D12_RESOURCE_STATE_COPY_DEST);

		// コピーコマンドの発行
		a_pCmdList->CopyBufferRegion(
			m_deadList.GetResource(), 0,
			_deadListUpload.GetResource(), 0,
			m_maxCapacity * sizeof(uint32_t)
		);
		a_pCmdList->CopyBufferRegion(
			m_counterBuffer.GetResource(), 0,
			_counterUpload.GetResource(), 0,
			sizeof(uint32_t)
		);

		// コピー後のリソースバリア (DEFAULTヒープを UAV 状態にする)
		m_deadList.Barrier(a_pCmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		m_counterBuffer.Barrier(a_pCmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

		// 遅延開放のためアップロードヒープをリソースマネージャーに預ける
		Resource::ResourceManager::Instance().RegisterUploadBuffer(_deadListUpload.GetResource());
		Resource::ResourceManager::Instance().RegisterUploadBuffer(_counterUpload.GetResource());


	}
}