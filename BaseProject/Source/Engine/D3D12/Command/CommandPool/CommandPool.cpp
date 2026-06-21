#include "CommandPool.h"

namespace Engine::D3D12
{
	void Engine::D3D12::CommandPool::Init(Device* a_pDevice, D3D12_COMMAND_LIST_TYPE a_type)
	{

		m_type = a_type;
		// コマンドキュー作成
		D3D12_COMMAND_QUEUE_DESC _queueDesc = {};
		_queueDesc.Type = a_type;
		_queueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
		_queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
		_queueDesc.NodeMask = 0;

		// 作成
		HRESULT _hr = a_pDevice->CreateCommandQueue(
			&_queueDesc,
			IID_PPV_ARGS(m_cpCmdQueue.ReleaseAndGetAddressOf())
		);
		Engine::Debug::ErrLog(SUCCEEDED(_hr), "CommandQueue の生成に失敗 HRESULT:%08X", _hr);

		// フェンス作成
		_hr = a_pDevice->CreateFence(
			0,
			D3D12_FENCE_FLAG_NONE,
			IID_PPV_ARGS(m_cpFence.ReleaseAndGetAddressOf())
		);
		Engine::Debug::ErrLog(SUCCEEDED(_hr), "Fence の生成に失敗 HRESULT:%08X", _hr);

		m_fenceValue = 0;
	}

	void Engine::D3D12::CommandPool::Release()
	{
		// GPU完了を待って解放
		// キューとフェンスの存在チェック
		if (m_cpCmdQueue && m_cpFence)
		{
			// GPU完了待ち
			++m_fenceValue;
			m_cpCmdQueue->Signal(m_cpFence.Get(), m_fenceValue);

			// フェンスの値が目標ちを超えていたら
			if (m_cpFence->GetCompletedValue() < m_fenceValue)
			{
				HANDLE _event = CreateEvent(nullptr, FALSE, FALSE, nullptr);
				m_cpFence->SetEventOnCompletion(m_fenceValue, _event);
				WaitForSingleObject(_event,INFINITE);
			}
		}

		// 各配列のクリア
		m_freeLists.clear();
		m_inFlightLists.clear();

		// ComPtrの解放
		m_cpFence.Reset();
		m_cpCmdQueue.Reset();
	}

	GraphicsCommandList* CommandPool::AcquireList(Device* a_pDevice, ID3D12CommandAllocator* a_pAllocator)
	{
		// 完了済みのものをフリーに戻す
		UINT64 _completed = m_cpFence->GetCompletedValue();
		for (auto it = m_inFlightLists.begin(); it != m_inFlightLists.end();)
		{
			// フェンス値が目標値を超えていたら
			if (it->fenceValue <= _completed)
			{
				// 使用済みになったものをフリーリストに返す
				m_freeLists.push_back(std::move(it->cpList));
				it = m_inFlightLists.erase(it);
			}
			// そうでなければ処理中なので次のリストを見に行く
			else
			{ 
				++it;
			}
		}

		// フリーがあれば返す、なければ新規作成
		if (!m_freeLists.empty())
		{
			// フリーリストから取得
			ComPtr<GraphicsCommandList> _cpList = std::move(m_freeLists.back());
			m_freeLists.pop_back();			// 取得したのを配列から消す

			GraphicsCommandList* _pRaw = _cpList.Get();

			// フリーリストの再利用時は、渡されたアロケーターでリセットする
			auto _hr = _pRaw->Reset(a_pAllocator, nullptr);
			Debug::ErrLog(SUCCEEDED(_hr),"ComandListのリセットに失敗");

			// 生ポインタと ComPtrを紐づける
			m_trackingMap[_pRaw] = std::move(_cpList);
			return _pRaw;
		}
		

		// フリーがなければ新規作成
		ComPtr<GraphicsCommandList> _cpNewList;
		HRESULT _hr = a_pDevice->CreateCommandList(
			0,
			m_type,
			a_pAllocator,
			nullptr,
			IID_PPV_ARGS(_cpNewList.ReleaseAndGetAddressOf())
		);
		Engine::Debug::ErrLog(SUCCEEDED(_hr), "CommandList の生成に失敗 HRESULT:%08X", _hr);

		// 作成後にマップに登録して返す
		GraphicsCommandList* _pRaw = _cpNewList.Get();
		m_trackingMap[_pRaw] = std::move(_cpNewList);
		return _pRaw;
	}

	void CommandPool::SubmitList(GraphicsCommandList* a_pList)
	{
		// 待機状態にする
		a_pList->Close();

		std::lock_guard<std::mutex> _lock(m_mutex);
		m_pendingLists.push_back(a_pList);
	}

	UINT64 CommandPool::ExecutePendingLists()
	{
		std::lock_guard<std::mutex> _lock(m_mutex);

		// 実行待ちがなければリターン
		if (m_pendingLists.empty()) return m_fenceValue;

		// APIに渡すための配列を作成
		std::vector<ID3D12CommandList*> _executeLists;
		_executeLists.reserve(m_pendingLists.size());
		for (GraphicsCommandList* _pList : m_pendingLists)
		{
			_executeLists.push_back(_pList);
		}

		// 一括実行
		m_cpCmdQueue->ExecuteCommandLists(static_cast<UINT>(_executeLists.size()), _executeLists.data());

		// フェンスを進める
		++m_fenceValue;
		m_cpCmdQueue->Signal(m_cpFence.Get(), m_fenceValue);

		// InFlight (実行中) リストへ移動
		for (GraphicsCommandList* _pList : m_pendingLists)
		{
			auto _it = m_trackingMap.find(_pList);
			if (_it != m_trackingMap.end())
			{
				m_inFlightLists.push_back({ std::move(_it->second), m_fenceValue });
				m_trackingMap.erase(_it);
			}
		}

		// 実行待ちリストをクリア
		m_pendingLists.clear();

		return m_fenceValue;
	}

	void CommandPool::ExecuteImmediate(GraphicsCommandList* a_pList)
	{
		// コマンドリストを閉じる
		a_pList->Close();

		ID3D12CommandList* _lists[] = { a_pList };
		m_cpCmdQueue->ExecuteCommandLists(1, _lists);

		// 完了まで待つ
		++m_fenceValue;
		m_cpCmdQueue->Signal(m_cpFence.Get(), m_fenceValue);

		HANDLE _event = CreateEvent(nullptr, FALSE, FALSE, nullptr);
		m_cpFence->SetEventOnCompletion(m_fenceValue, _event);
		WaitForSingleObject(_event, INFINITE);
		CloseHandle(_event);

		// プールに返却
		std::lock_guard<std::mutex> _lock(m_mutex);
		auto _it = m_trackingMap.find(a_pList);
		Debug::ErrLog(_it != m_trackingMap.end(), "ExecuteAndRelease : 未知のコマンドリストが渡されました");

		m_freeLists.push_back(std::move(_it->second));
		m_trackingMap.erase(_it);
	}
}
