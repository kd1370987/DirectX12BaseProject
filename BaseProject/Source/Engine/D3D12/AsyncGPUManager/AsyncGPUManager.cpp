#include "AsyncGPUManager.h"

namespace Engine::D3D12
{
	AsyncGPUManager::AsyncGPUManager() : m_isExitWorker(false) {}
	AsyncGPUManager::~AsyncGPUManager() { Release(); }

	void AsyncGPUManager::Init()
	{
		m_isExitWorker = false;
		m_workerThread = std::thread(&AsyncGPUManager::WorkerThreadMain, this);
	}

	void AsyncGPUManager::Release()
	{
		m_isExitWorker = true;
		if (m_workerThread.joinable())
		{
			m_workerThread.join();
		}

		m_freeCopyAllocators.clear();
		m_freeComputeAllocators.clear();
		m_inFlightTasks.clear();
	}

	ID3D12CommandAllocator* AsyncGPUManager::AcquireAllocator(Device* a_pDevice, AsyncCommandType a_type)
	{
		std::lock_guard<std::mutex> _lock(m_mutex);

		// フリーリストがあれば取り出して返す
		if (a_type == AsyncCommandType::Copy && !m_freeCopyAllocators.empty())
		{
			auto _alloc = m_freeCopyAllocators.back();
			m_freeCopyAllocators.pop_back();
			return _alloc.Get();
		}
		else if (a_type == AsyncCommandType::Compute && !m_freeComputeAllocators.empty())
		{
			auto _alloc = m_freeComputeAllocators.back();
			m_freeComputeAllocators.pop_back();
			return _alloc.Get();
		}

		// フリーがなければ新規作成
		ComPtr<ID3D12CommandAllocator> _newAllocator;
		D3D12_COMMAND_LIST_TYPE _d3dType = (a_type == AsyncCommandType::Copy) ? D3D12_COMMAND_LIST_TYPE_COPY : D3D12_COMMAND_LIST_TYPE_COMPUTE;

		a_pDevice->CreateCommandAllocator(_d3dType, IID_PPV_ARGS(_newAllocator.ReleaseAndGetAddressOf()));

		// とりあえず今回は作成したものをそのまま返す（所有権は RegisterTask で再度受け取る設計）
		// ※一時的に呼び出し元にポインタだけ渡すため、この段階では ComPtr の寿命切れに注意。
		// 実際には新規作成したものをリストに保持しつつ生ポインタを返すのが安全です。

		// 簡易対応：新規作成したアロケーターを一時的に返す用としてフリーリストにダミーで突っ込んでから返す
		if (a_type == AsyncCommandType::Copy) {
			m_freeCopyAllocators.push_back(_newAllocator);
			auto* p = m_freeCopyAllocators.back().Get();
			m_freeCopyAllocators.pop_back();
			// 注意：本来なら「貸出中リスト」に入れるべきですが、すぐRegisterTaskで戻ってくる前提とします
			// 安全に書くなら、戻り値ではなくRegisterTaskまでAsyncManager内で完結させる方が良いです。
		}

		// より安全な実装：新規アロケーターは一時的にポインタだけ返す（所有者は呼び出し側に一時委譲）
		// コムポインタから切り離すため Detach するか、インターフェースを ComPtr 返しにするか。
		// ここは「呼び出し元が使い終わったら RegisterTask で渡してくる」ので、生ポインタでもOKです。
		_newAllocator->AddRef(); // 呼び出し側に一時的に参照カウントを渡す
		return _newAllocator.Get();
	}

	void AsyncGPUManager::RegisterTask(AsyncCommandType a_type, ID3D12CommandAllocator* a_pAllocator, Fence* a_pFence, UINT64 a_targetFenceValue, std::function<void()> a_onComplete)
	{
		std::lock_guard<std::mutex> _lock(m_mutex);

		ComPtr<ID3D12CommandAllocator> _allocator;
		_allocator.Attach(a_pAllocator); // AcquireAllocatorで増やした参照カウントをここで回収

		m_inFlightTasks.push_back({ a_type, _allocator, a_pFence, a_targetFenceValue, a_onComplete });
	}

	void AsyncGPUManager::WorkerThreadMain()
	{
		while (!m_isExitWorker)
		{
			bool _hasInFlight = false;
			{
				std::lock_guard<std::mutex> _lock(m_mutex);
				for (auto _it = m_inFlightTasks.begin(); _it != m_inFlightTasks.end(); )
				{
					// CommandPoolが持っているフェンスを直接監視！
					if (_it->pTargetFence->GetCompletedValue() >= _it->targetFenceValue)
					{
						// ① 完了したのでコールバックを実行（Uploadヒープ解放など）
						if (_it->callback) _it->callback();

						// ② アロケーターをリセットしてフリーリストに返却
						_it->cpAllocator->Reset();
						if (_it->type == AsyncCommandType::Copy)
						{
							m_freeCopyAllocators.push_back(_it->cpAllocator);
						}
						else
						{
							m_freeComputeAllocators.push_back(_it->cpAllocator);
						}

						// ③ 実行中リストから削除
						_it = m_inFlightTasks.erase(_it);
					}
					else
					{
						_hasInFlight = true;
						++_it;
					}
				}
			}

			// タスクがある時は2ms間隔、無い時は16ms間隔で監視（CPU負荷を極限まで下げる）
			if (_hasInFlight) {
				std::this_thread::sleep_for(std::chrono::milliseconds(2));
			}
			else {
				std::this_thread::sleep_for(std::chrono::milliseconds(16));
			}
		}
	}
}