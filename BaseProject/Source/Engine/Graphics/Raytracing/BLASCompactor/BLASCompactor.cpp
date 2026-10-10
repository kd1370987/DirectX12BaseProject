#include "Engine/Graphics/Raytracing/BLASCompactor/BLASCompactor.h"

#include "Engine/Graphics/Device/FrameManager/FrameManager.h"

namespace Engine::Graphics::Raytracing
{
	namespace
	{
		// バッファ 1 本を committed で作る
		ComPtr<ID3D12Resource> CreateBuffer(
			D3D12::Device* a_pDevice,
			D3D12_HEAP_TYPE a_heapType,
			UINT64 a_size,
			D3D12_RESOURCE_FLAGS a_flags,
			D3D12_RESOURCE_STATES a_initialState,
			LPCWSTR a_name
		)
		{
			D3D12_HEAP_PROPERTIES _heapProps = {};
			_heapProps.Type = a_heapType;

			D3D12_RESOURCE_DESC _desc = {};
			_desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
			_desc.Width = a_size;
			_desc.Height = 1;
			_desc.DepthOrArraySize = 1;
			_desc.MipLevels = 1;
			_desc.Format = DXGI_FORMAT_UNKNOWN;
			_desc.SampleDesc.Count = 1;
			_desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
			_desc.Flags = a_flags;

			ComPtr<ID3D12Resource> _cpResource = nullptr;
			const HRESULT _hr = a_pDevice->CreateCommittedResource(
				&_heapProps, D3D12_HEAP_FLAG_NONE, &_desc, a_initialState, nullptr,
				IID_PPV_ARGS(_cpResource.ReleaseAndGetAddressOf()));
			if (FAILED(_hr)) return nullptr;

			_cpResource->SetName(a_name);	// リーク調査用
			return _cpResource;
		}
	}

	BLASCompactor::~BLASCompactor()
	{
		Release();
	}

	bool BLASCompactor::Init(D3D12::Device* a_pDevice, const FrameManager* a_pFrameManager)
	{
		if (!a_pDevice || !a_pFrameManager) return false;

		m_pDevice = a_pDevice;
		m_pFrameManager = a_pFrameManager;
		m_spInbox = std::make_shared<Inbox>();

		// 圧縮後の大きさは UAV にしか書けないので、GPU 側に書かせてから読み戻し用へ写す
		const UINT64 _bufferSize = SLOT_SIZE * SLOT_COUNT;
		m_cpPostbuildInfo = CreateBuffer(m_pDevice, D3D12_HEAP_TYPE_DEFAULT, _bufferSize,
			D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COMMON, L"BLASCompactor_PostbuildInfo");
		m_cpReadback = CreateBuffer(m_pDevice, D3D12_HEAP_TYPE_READBACK, _bufferSize,
			D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_COPY_DEST, L"BLASCompactor_Readback");
		if (!m_cpPostbuildInfo || !m_cpReadback)
		{
			ENGINE_ERRLOG(false, "[BLASCompactor] 圧縮後の大きさを受け取るバッファを作れませんでした");
			Release();
			return false;
		}
		m_postbuildInfoState = D3D12_RESOURCE_STATE_COMMON;

		D3D12::VideoMemoryTracker::TrackResource(m_cpPostbuildInfo.Get(), D3D12::EVideoMemoryCategory::BLASScratch);
		D3D12::VideoMemoryTracker::TrackResource(m_cpReadback.Get(), D3D12::EVideoMemoryCategory::BLASScratch);

		// 読み戻し用は作りっぱなしで開いておく。読むのは GPU がそのフレームを終えてから
		void* _pMapped = nullptr;
		if (FAILED(m_cpReadback->Map(0, nullptr, &_pMapped)) || !_pMapped)
		{
			ENGINE_ERRLOG(false, "[BLASCompactor] 読み戻し用のバッファを開けませんでした");
			Release();
			return false;
		}
		m_pReadbackData = static_cast<const UINT64*>(_pMapped);

		m_freeSlotVec.clear();
		m_freeSlotVec.reserve(SLOT_COUNT);
		for (uint32_t _i = SLOT_COUNT; _i > 0; --_i)
		{
			m_freeSlotVec.push_back(_i - 1);
		}
		return true;
	}

	void BLASCompactor::Release()
	{
		// 受け口を消す : この後に届いた完了通知は何もしない
		m_spInbox.reset();
		m_jobVec.clear();

		// 終了処理は GPU を止めてから来るので、使い終わりを待たずに手放してよい
		m_retiredVec.clear();

		if (m_cpReadback && m_pReadbackData) m_cpReadback->Unmap(0, nullptr);
		m_pReadbackData = nullptr;
		m_cpReadback.Reset();
		m_cpPostbuildInfo.Reset();
		m_freeSlotVec.clear();

		m_pDevice = nullptr;
		m_pFrameManager = nullptr;
	}

	std::function<void()> BLASCompactor::MakeBuildCompleteNotifier(const std::shared_ptr<BLASCompactionTarget>& a_spTarget) const
	{
		// どちらも弱参照で持つ : 通知が来るまでに BLAS やこの係が消えていれば何もしない
		std::weak_ptr<Inbox> _wpInbox = m_spInbox;
		std::weak_ptr<BLASCompactionTarget> _wpTarget = a_spTarget;

		return [_wpInbox, _wpTarget]()
			{
				std::shared_ptr<Inbox> _spInbox = _wpInbox.lock();
				if (!_spInbox) return;

				std::lock_guard<std::mutex> _lock(_spInbox->mutex);
				_spInbox->targetVec.push_back(_wpTarget);
			};
	}

	void BLASCompactor::Execute(D3D12::GraphicsCommandList* a_pCmdList)
	{
		if (!m_pDevice || !a_pCmdList || !m_pReadbackData) return;

		ReleaseRetired();
		TakeInbox();
		if (m_jobVec.empty()) return;

		// 先に差し替えを済ませる : 席が空くので、同じフレームで次の問い合わせに回せる
		RecordCompactions(a_pCmdList);
		RecordSizeQueries(a_pCmdList);
	}

	void BLASCompactor::TakeInbox()
	{
		if (!m_spInbox) return;

		std::vector<std::weak_ptr<BLASCompactionTarget>> _targetVec = {};
		{
			std::lock_guard<std::mutex> _lock(m_spInbox->mutex);
			_targetVec.swap(m_spInbox->targetVec);
		}

		for (auto& _wpTarget : _targetVec)
		{
			Job _job = {};
			_job.wpTarget = std::move(_wpTarget);
			m_jobVec.push_back(std::move(_job));
		}
	}

	//======================================================================================
	// 圧縮後の大きさを書かせる
	//
	// 大きさは UAV にしか書けないので、席ごとに書かせてから読み戻し用へ写す。
	// 写した値が読めるのは、このフレームを GPU が終えてから
	//======================================================================================
	void BLASCompactor::RecordSizeQueries(D3D12::GraphicsCommandList* a_pCmdList)
	{
		const uint64_t _readyFenceValue = m_pFrameManager->GetNextFenceValue();
		const D3D12_GPU_VIRTUAL_ADDRESS _postbuildBase = m_cpPostbuildInfo->GetGPUVirtualAddress();

		std::vector<uint32_t> _issuedSlotVec = {};

		for (auto _it = m_jobVec.begin(); _it != m_jobVec.end();)
		{
			Job& _job = *_it;

			// 依頼済みのものは、大きさが読めるようになるのを待つ
			if (_job.readyFenceValue != 0)
			{
				++_it;
				continue;
			}

			// BLAS が先に消えていたら手を引く
			std::shared_ptr<BLASCompactionTarget> _spTarget = _job.wpTarget.lock();
			std::unique_lock<std::mutex> _lock = {};
			if (_spTarget) _lock = std::unique_lock<std::mutex>(_spTarget->mutex);
			if (!_spTarget || !_spTarget->cpResource)
			{
				ReleaseSlot(_job.slot);
				_it = m_jobVec.erase(_it);
				continue;
			}

			// 席が尽きたら、残りは次のフレームへ回す
			if (_job.slot == INVALID_SLOT) _job.slot = AcquireSlot();
			if (_job.slot == INVALID_SLOT) break;

			// 書き込む前に UAV へ
			if (m_postbuildInfoState != D3D12_RESOURCE_STATE_UNORDERED_ACCESS)
			{
				D3D12::ResourceBarrier(a_pCmdList, m_cpPostbuildInfo.Get(), m_postbuildInfoState, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
				m_postbuildInfoState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
			}

			D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC _infoDesc = {};
			_infoDesc.DestBuffer = _postbuildBase + SLOT_SIZE * _job.slot;
			_infoDesc.InfoType = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_COMPACTED_SIZE;

			const D3D12_GPU_VIRTUAL_ADDRESS _source = _spTarget->cpResource->GetGPUVirtualAddress();
			a_pCmdList->EmitRaytracingAccelerationStructurePostbuildInfo(&_infoDesc, 1, &_source);

			_job.readyFenceValue = _readyFenceValue;
			_issuedSlotVec.push_back(_job.slot);
			++_it;
		}

		if (_issuedSlotVec.empty()) return;

		// 書かせた席だけを読み戻し用へ写す。
		// UAV からコピー元への遷移が、書き込みの完了待ちを兼ねる
		D3D12::ResourceBarrier(a_pCmdList, m_cpPostbuildInfo.Get(), m_postbuildInfoState, D3D12_RESOURCE_STATE_COPY_SOURCE);
		m_postbuildInfoState = D3D12_RESOURCE_STATE_COPY_SOURCE;

		for (const uint32_t _slot : _issuedSlotVec)
		{
			const UINT64 _offset = SLOT_SIZE * _slot;
			a_pCmdList->CopyBufferRegion(m_cpReadback.Get(), _offset, m_cpPostbuildInfo.Get(), _offset, SLOT_SIZE);
		}

		// バッファはコマンドリストを流し終えると COMMON に戻る。
		// 覚えている状態と食い違わないよう、こちらからも COMMON へ戻しておく
		D3D12::ResourceBarrier(a_pCmdList, m_cpPostbuildInfo.Get(), m_postbuildInfoState, D3D12_RESOURCE_STATE_COMMON);
		m_postbuildInfoState = D3D12_RESOURCE_STATE_COMMON;
	}

	//======================================================================================
	// 圧縮して差し替える
	//
	// 古い実体は、今フレームの圧縮コピーと、前のフレームの TLAS が読み終わるまで生かしておく。
	// 差し替えた後に作る TLAS は新しい実体を指すので、今フレームの終わりで手放してよい
	//======================================================================================
	void BLASCompactor::RecordCompactions(D3D12::GraphicsCommandList* a_pCmdList)
	{
		const uint64_t _completedFenceValue = m_pFrameManager->GetCompletedFenceValue();
		const uint64_t _retireFenceValue = m_pFrameManager->GetNextFenceValue();

		bool _isAnyCopied = false;
		const uint32_t _beforeCount = m_compactedCount;
		const uint64_t _beforeSavedBytes = m_savedBytes;

		for (auto _it = m_jobVec.begin(); _it != m_jobVec.end();)
		{
			Job& _job = *_it;

			// まだ問い合わせていないもの・GPU が書き終えていないものは待つ
			if (_job.readyFenceValue == 0 || _completedFenceValue < _job.readyFenceValue)
			{
				++_it;
				continue;
			}

			const UINT64 _compactedSize = m_pReadbackData[_job.slot];
			ReleaseSlot(_job.slot);

			std::shared_ptr<BLASCompactionTarget> _spTarget = _job.wpTarget.lock();
			if (_spTarget)
			{
				std::lock_guard<std::mutex> _lock(_spTarget->mutex);
				ID3D12Resource* _pSource = _spTarget->cpResource.Get();
				const UINT64 _sourceSize = _pSource ? _pSource->GetDesc().Width : 0;

				// 小さくならないなら作り直さない
				if (_pSource && _compactedSize > 0 && _compactedSize < _sourceSize)
				{
					ComPtr<ID3D12Resource> _cpCompacted = CreateBuffer(
						m_pDevice, D3D12_HEAP_TYPE_DEFAULT, _compactedSize,
						D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
						D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE,
						L"BLAS_Compacted");

					if (_cpCompacted)
					{
						D3D12::VideoMemoryTracker::TrackResource(_cpCompacted.Get(), D3D12::EVideoMemoryCategory::BLAS);

						a_pCmdList->CopyRaytracingAccelerationStructure(
							_cpCompacted->GetGPUVirtualAddress(),
							_pSource->GetGPUVirtualAddress(),
							D3D12_RAYTRACING_ACCELERATION_STRUCTURE_COPY_MODE_COMPACT);

						Retired _retired = {};
						_retired.cpResource = std::move(_spTarget->cpResource);
						_retired.fenceValue = _retireFenceValue;
						m_retiredVec.push_back(std::move(_retired));

						_spTarget->cpResource = std::move(_cpCompacted);

						++m_compactedCount;
						m_savedBytes += _sourceSize - _compactedSize;
						_isAnyCopied = true;
					}
				}
			}

			_it = m_jobVec.erase(_it);
		}

		// 圧縮コピーの書き込みを、後に続く TLAS のビルドとレイトレが読む前に終わらせる
		if (_isAnyCopied)
		{
			D3D12_RESOURCE_BARRIER _barrier = {};
			_barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
			_barrier.UAV.pResource = nullptr;
			a_pCmdList->ResourceBarrier(1, &_barrier);

			ENGINE_LOG("[BLASCompactor] %u 件を圧縮 (%.2f MB 減) : 累計 %u 件・%.2f MB 減",
				m_compactedCount - _beforeCount,
				static_cast<double>(m_savedBytes - _beforeSavedBytes) / (1024.0 * 1024.0),
				m_compactedCount,
				static_cast<double>(m_savedBytes) / (1024.0 * 1024.0));
		}
	}

	void BLASCompactor::ReleaseRetired()
	{
		const uint64_t _completedFenceValue = m_pFrameManager->GetCompletedFenceValue();

		std::erase_if(m_retiredVec, [_completedFenceValue](const Retired& a_retired)
			{
				return a_retired.fenceValue <= _completedFenceValue;
			});
	}

	uint32_t BLASCompactor::AcquireSlot()
	{
		if (m_freeSlotVec.empty()) return INVALID_SLOT;

		const uint32_t _slot = m_freeSlotVec.back();
		m_freeSlotVec.pop_back();
		return _slot;
	}

	void BLASCompactor::ReleaseSlot(uint32_t a_slot)
	{
		if (a_slot == INVALID_SLOT) return;
		m_freeSlotVec.push_back(a_slot);
	}
}
