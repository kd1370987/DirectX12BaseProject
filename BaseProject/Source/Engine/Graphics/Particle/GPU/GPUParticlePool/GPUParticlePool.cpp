#include "Engine/Graphics/Particle/GPU/GPUParticlePool/GPUParticlePool.h"

#include <numeric>	// std::iota

#include "Engine/MainEngine.h"

#include "Engine/Resource/Manager/ResourceManager/ResourceManager.h"

namespace Engine::Graphics::Particle
{
	bool Engine::Graphics::Particle::GPUParticlePool::Init(
		Graphics::D3D12::Device* a_pDevice,
		Graphics::D3D12::DescriptorHeapManager* a_pHeapManager,
		Graphics::D3D12::GraphicsCommandList* a_pCmdList,
		Engine::Handle<Resource::ParticlesAsset> a_particleHandle,
		UINT a_capacity
	)
	{
		// 容量は呼ぶ側が ToInitialCapacity で決める(0 個のバッファは作らない)
		m_maxCapacity = (std::max)(a_capacity, 1u);
		m_initialCapacity = m_maxCapacity;
		m_growFromCapacity = m_maxCapacity;
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

		// ビデオメモリの集計ではパーティクルとして数える
		Graphics::D3D12::GPUResource* const _pBuffers[] = { &m_particlePool, &m_deadList, &m_counterBuffer, &m_drawArgs, &m_aliveList };
		for (Graphics::D3D12::GPUResource* _pBuffer : _pBuffers)
		{
			_pBuffer->SetMemoryCategory(Graphics::D3D12::EVideoMemoryCategory::Particle);
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
		std::shared_ptr<Graphics::D3D12::DynamicBuffer> _spDeadListUpload = std::make_shared<Graphics::D3D12::DynamicBuffer>();
		std::shared_ptr<Graphics::D3D12::DynamicBuffer> _spCounterUpload = std::make_shared<Graphics::D3D12::DynamicBuffer>();
		std::shared_ptr<Graphics::D3D12::DynamicBuffer> _spParticleUpload = std::make_shared<Graphics::D3D12::DynamicBuffer>();

		Graphics::D3D12::DynamicBufferDesc _deadDesc = { m_maxCapacity, sizeof(uint32_t), D3D12_RESOURCE_FLAG_NONE };
		Graphics::D3D12::DynamicBufferDesc _countDesc = { 1, sizeof(uint32_t), D3D12_RESOURCE_FLAG_NONE };
		Graphics::D3D12::DynamicBufferDesc _particleDesc = { m_maxCapacity, sizeof(ParticleData), D3D12_RESOURCE_FLAG_NONE };

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
		MainEngine::Instance().ReserveRelease([_spDeadListUpload,_spCounterUpload,_spParticleUpload]()
			{
				_spDeadListUpload->Release();
				_spCounterUpload->Release();
				_spParticleUpload->Release();
			});

		return true;
	}

	//======================================================================================
	// 容量を伸ばす : 作り直して古い中身を先頭へ写す
	//
	// 状態の約束 : フレームの頭は COMMON(粒・デッドリスト・カウンターは、発生と更新が
	// 暗黙の昇格で使っている)。ここでは写す・埋めるのに明示して遷移させ、EndGrow で COMMON へ戻す
	//======================================================================================
	bool GPUParticlePool::BeginGrow(
		Graphics::D3D12::Device* a_pDevice,
		Graphics::D3D12::DescriptorHeapManager* a_pHeapManager,
		Graphics::D3D12::GraphicsCommandList* a_pCmdList,
		UINT a_newCapacity)
	{
		if (!a_pDevice || !a_pHeapManager || !a_pCmdList) return false;
		if (a_newCapacity <= m_maxCapacity) return false;
		if (!m_particlePool.GetResource() || !m_deadList.GetResource() || !m_counterBuffer.GetResource()) return false;

		//------------------------------------------------------------------
		// 新しい3本を作る
		//------------------------------------------------------------------
		Graphics::D3D12::RWStructuredBuffer<ParticleData> _newPool;
		Graphics::D3D12::RWStructuredBuffer<uint32_t> _newDeadList;
		Graphics::D3D12::RWStructuredBuffer<uint32_t> _newAliveList;
		_newPool.Create(a_pDevice, a_pHeapManager, a_newCapacity);
		_newDeadList.Create(a_pDevice, a_pHeapManager, a_newCapacity);
		_newAliveList.Create(a_pDevice, a_pHeapManager, a_newCapacity);

		if (!_newPool.GetResource() || !_newDeadList.GetResource() || !_newAliveList.GetResource())
		{
			// 作れたぶんも返す(ディスクリプタが漏れないように)
			_newPool.Release();
			_newDeadList.Release();
			_newAliveList.Release();
			return false;
		}
		_newAliveList.GetResource()->SetName(L"ParticleAliveList");
		_newPool.SetMemoryCategory(Graphics::D3D12::EVideoMemoryCategory::Particle);
		_newDeadList.SetMemoryCategory(Graphics::D3D12::EVideoMemoryCategory::Particle);
		_newAliveList.SetMemoryCategory(Graphics::D3D12::EVideoMemoryCategory::Particle);

		//------------------------------------------------------------------
		// 古い粒とデッドリストを、新しい先頭へ写す。
		// デッドリストは [0, カウンター) が空き番号のスタック。全部写しておけば、
		// その上へ新しい空き番号を積むだけで済む(カウンターは GPU にしか無いので、積むのは CS)
		//------------------------------------------------------------------
		m_particlePool.Barrier(a_pCmdList, D3D12_RESOURCE_STATE_COPY_SOURCE);
		m_deadList.Barrier(a_pCmdList, D3D12_RESOURCE_STATE_COPY_SOURCE);
		_newPool.Barrier(a_pCmdList, D3D12_RESOURCE_STATE_COPY_DEST);
		_newDeadList.Barrier(a_pCmdList, D3D12_RESOURCE_STATE_COPY_DEST);

		a_pCmdList->CopyBufferRegion(
			_newPool.GetResource(), 0,
			m_particlePool.GetResource(), 0,
			static_cast<UINT64>(m_maxCapacity) * sizeof(ParticleData));
		a_pCmdList->CopyBufferRegion(
			_newDeadList.GetResource(), 0,
			m_deadList.GetResource(), 0,
			static_cast<UINT64>(m_maxCapacity) * sizeof(uint32_t));

		// 増えた範囲は埋める CS が書くので UAV へ。カウンターも CS が足す
		_newPool.Barrier(a_pCmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		_newDeadList.Barrier(a_pCmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		m_counterBuffer.Barrier(a_pCmdList, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

		//------------------------------------------------------------------
		// 古い3本は、このフレームのコピーと前のフレームの描画が読み終わってから返す。
		// 壊すだけではディスクリプタが返らないので Release() を呼ぶ
		//------------------------------------------------------------------
		struct OldBuffers
		{
			Graphics::D3D12::RWStructuredBuffer<ParticleData> pool;
			Graphics::D3D12::RWStructuredBuffer<uint32_t> deadList;
			Graphics::D3D12::RWStructuredBuffer<uint32_t> aliveList;
		};
		auto _spOld = std::make_shared<OldBuffers>();
		_spOld->pool = std::move(m_particlePool);
		_spOld->deadList = std::move(m_deadList);
		_spOld->aliveList = std::move(m_aliveList);
		MainEngine::Instance().ReserveRelease([_spOld]()
			{
				_spOld->pool.Release();
				_spOld->deadList.Release();
				_spOld->aliveList.Release();
			});

		m_particlePool = std::move(_newPool);
		m_deadList = std::move(_newDeadList);
		m_aliveList = std::move(_newAliveList);

		m_growFromCapacity = m_maxCapacity;
		m_maxCapacity = a_newCapacity;
		return true;
	}

	void GPUParticlePool::EndGrow(Graphics::D3D12::GraphicsCommandList* a_pCmdList)
	{
		if (!a_pCmdList) return;

		// 遷移のバリアが、埋める CS の書き込みの完了待ちも兼ねる
		m_particlePool.Barrier(a_pCmdList, D3D12_RESOURCE_STATE_COMMON);
		m_deadList.Barrier(a_pCmdList, D3D12_RESOURCE_STATE_COMMON);
		m_counterBuffer.Barrier(a_pCmdList, D3D12_RESOURCE_STATE_COMMON);
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