#include "GPUParticlePool.h"

#include <numeric>	// std::iota

#include "../../../../MainEngine.h"

#include "../../../../Resource/Manager/ResourceManager/ResourceManager.h"

namespace Engine::Particle
{
	bool Engine::Particle::GPUParticlePool::Init(
		D3D12::Device* a_pDevice,
		D3D12::DescriptorHeapManager* a_pHeapManager,
		D3D12::GraphicsCommandList* a_pCmdList,
		Engine::Handle<Resource::ParticlesAsset> a_particleHandle,
		const Resource::ResourceManager& a_resourceManager
	)
	{
		auto* _pParticleAsset = a_resourceManager.Get(a_particleHandle);
		if (!_pParticleAsset)
		{
			ENGINE_WARNING("パーティクルプールの作成に失敗 : パーティクルアセットが読み込めませんでした");
			return false;
		}

		// パーティクルデータの確保(容量は ToPoolCapacity で丸める。作り直しの判定と同じ丸め方)
		m_maxCapacity = ToPoolCapacity(_pParticleAsset->GetCapacity());
		m_assetHandle = a_particleHandle;

		// バッファの作成
		m_particlePool.Create(a_pDevice, a_pHeapManager, m_maxCapacity);
		m_deadList.Create(a_pDevice, a_pHeapManager, m_maxCapacity);
		m_counterBuffer.Create(a_pDevice, a_pHeapManager, 1);

		// 間接描画の引数(D3D12_DRAW_INDEXED_ARGUMENTS = uint ×5)。
		// 中身は毎フレーム、使う前にリセット用の CS が書くので初期値は要らない
		m_drawArgs.Create(a_pDevice, a_pHeapManager, DRAW_ARGS_ELEMENT_NUM);
		if (m_drawArgs.GetResource())
		{
			m_drawArgs.GetResource()->SetName(L"ParticleDrawArgs");	// PIX で見分けやすいように
		}
		m_isArgsReady = false;

		// 生存リスト(生きている粒の番号の一覧)。
		// Update が数えたぶんしか読まないので、初期値は要らない
		m_aliveList.Create(a_pDevice, a_pHeapManager, m_maxCapacity);
		if (m_aliveList.GetResource())
		{
			m_aliveList.GetResource()->SetName(L"ParticleAliveList");
		}

		// バッファの初期化用データの作成
		std::vector<uint32_t> _initDeadList(m_maxCapacity);
		std::iota(_initDeadList.begin(),_initDeadList.end(),0); // すべての配列を0から連番で埋めてくれる
		uint32_t _initCounter = m_maxCapacity;

		// パーティクル本体もゼロクリアする。
		// 作りっぱなしのVRAMには前の内容が残っている可能性があり、
		// life が 0 より大きいゴミが混ざると、出していないパーティクルが動き出したり、
		// それが寿命切れとしてデッドリストへ返却されてインデックスが二重登録される。
		std::vector<ParticleData> _initParticles(m_maxCapacity);

		// アップロードバッファを作成してコピーする
		std::shared_ptr<D3D12::DynamicBuffer> _spDeadListUpload = std::make_shared<D3D12::DynamicBuffer>();
		std::shared_ptr<D3D12::DynamicBuffer> _spCounterUpload = std::make_shared<D3D12::DynamicBuffer>();
		std::shared_ptr<D3D12::DynamicBuffer> _spParticleUpload = std::make_shared<D3D12::DynamicBuffer>();

		D3D12::DynamicBufferDesc _deadDesc = { m_maxCapacity, sizeof(uint32_t), D3D12_RESOURCE_FLAG_NONE };
		D3D12::DynamicBufferDesc _countDesc = { 1, sizeof(uint32_t), D3D12_RESOURCE_FLAG_NONE };
		D3D12::DynamicBufferDesc _particleDesc = { m_maxCapacity, sizeof(ParticleData), D3D12_RESOURCE_FLAG_NONE };

		_spDeadListUpload->Create(a_pDevice, a_pHeapManager, _deadDesc);
		_spCounterUpload->Create(a_pDevice, a_pHeapManager, _countDesc);
		_spParticleUpload->Create(a_pDevice, a_pHeapManager, _particleDesc);

		// データを書き込む
		_spDeadListUpload->UpdateData(_initDeadList.data(), m_maxCapacity * sizeof(uint32_t));
		_spCounterUpload->UpdateData(&_initCounter, sizeof(uint32_t));
		_spParticleUpload->UpdateData(_initParticles.data(), m_maxCapacity * sizeof(ParticleData));

		// コピーコマンドの発行。
		// コピー先のバッファは COMMON から COPY_DEST へ暗黙に昇格するので、コピー前のバリアは張らない
		a_pCmdList->CopyBufferRegion(
			m_deadList.GetResource(), 0,
			_spDeadListUpload->GetResource(), 0,
			m_maxCapacity * sizeof(uint32_t)
		);
		a_pCmdList->CopyBufferRegion(
			m_counterBuffer.GetResource(), 0,
			_spCounterUpload->GetResource(), 0,
			sizeof(uint32_t)
		);
		a_pCmdList->CopyBufferRegion(
			m_particlePool.GetResource(), 0,
			_spParticleUpload->GetResource(), 0,
			m_maxCapacity * sizeof(ParticleData)
		);

		// 解放処理を登録。
		// 壊すだけではディスクリプタが返らない(DynamicBuffer::Create はアップロードバッファにも SRV を取る)。
		// Release() を呼ばないと、プールを1つ作るたびにヒープの席が3つ漏れる
		MainEngine::Instance().RegisterDeferredResource([_spDeadListUpload,_spCounterUpload,_spParticleUpload]()
			{
				_spDeadListUpload->Release();
				_spCounterUpload->Release();
				_spParticleUpload->Release();
			});

		return true;
	}

	void GPUParticlePool::Release()
	{
		m_particlePool.Release();
		m_deadList.Release();
		m_counterBuffer.Release();
		m_drawArgs.Release();
		m_aliveList.Release();
		m_isArgsReady = false;
	}
}