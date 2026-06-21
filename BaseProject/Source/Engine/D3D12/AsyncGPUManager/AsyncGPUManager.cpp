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

		// フリーリストがあれば、所有権を剥奪（Detach）してそのまま返す
		if (a_type == AsyncCommandType::Copy && !m_freeCopyAllocators.empty())
		{
			auto _alloc = m_freeCopyAllocators.back();
			m_freeCopyAllocators.pop_back();
			return _alloc.Detach(); // ★修正：参照カウントを保ったままポインタを渡す
		}
		else if (a_type == AsyncCommandType::Compute && !m_freeComputeAllocators.empty())
		{
			auto _alloc = m_freeComputeAllocators.back();
			m_freeComputeAllocators.pop_back();
			return _alloc.Detach(); // ★修正
		}

		// フリーリストがなければ新規作成
		ComPtr<ID3D12CommandAllocator> _newAllocator;
		D3D12_COMMAND_LIST_TYPE _d3dType = (a_type == AsyncCommandType::Copy) ? D3D12_COMMAND_LIST_TYPE_COPY : D3D12_COMMAND_LIST_TYPE_COMPUTE;
		//D3D12_COMMAND_LIST_TYPE _d3dType = D3D12_COMMAND_LIST_TYPE_DIRECT;

		a_pDevice->CreateCommandAllocator(_d3dType, IID_PPV_ARGS(_newAllocator.ReleaseAndGetAddressOf()));

		// 作成したばかりのものも、所有権を剥奪（Detach）して一時的に外に預ける
		return _newAllocator.Detach(); // ★修正
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