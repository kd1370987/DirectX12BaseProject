#include "FrameManager.h"

namespace Engine::D3D12
{
	void FrameManager::Init(Device* a_pDevice)
	{
		// フレームリソースの生成
		for (auto& _res : m_frameResources)
		{
			// コマンドアロケーター作成
			HRESULT _hr = a_pDevice->CreateCommandAllocator(
				D3D12_COMMAND_LIST_TYPE_DIRECT,
				IID_PPV_ARGS(_res.cpAllocator.ReleaseAndGetAddressOf())
			);
			ENGINE_ERRLOG(SUCCEEDED(_hr),"CommandAllocater の生成に失敗 HRESULT:%08X", _hr);

			// フェンスバリュー初期化
			_res.fenceValue = 0;
		}

		// フェンス作成
		HRESULT _hr = a_pDevice->CreateFence(
			0,
			D3D12_FENCE_FLAG_NONE,
			IID_PPV_ARGS(m_cpFence.ReleaseAndGetAddressOf())
		);
		ENGINE_ERRLOG(SUCCEEDED(_hr), "Fence の生成に失敗 HRESULT:%08X", _hr);

		// 初回のため進めておく
		m_frameResources[m_cpuFrameIndex].fenceValue++;

		// フェンスイベント作成
		m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	}

	void FrameManager::Release()
	{
		// 全GPU作業が終わるのを待ってから解放
		WaitForAll();

		// イベント解放
		if (m_fenceEvent)
		{
			CloseHandle(m_fenceEvent);
			m_fenceEvent = nullptr;
		}

		// フレームリソースの解放
		for (auto& _res : m_frameResources)
		{
			_res.cpAllocator.Reset();
			_res.fenceValue = 0;
		}

		// フェンスの解放
		m_cpFence.Reset();
	}

	void FrameManager::BeginFrame()
	{
		// フレームインデックスの更新
		m_cpuFrameIndex = (m_cpuFrameIndex + 1) % static_cast<UINT>(CPU_FRAME_COUNT);
		WaitForFrame();

		// 完了が確認できたのでアロケーターをリセット
		m_frameResources[m_cpuFrameIndex].cpAllocator->Reset();
	}

	void FrameManager::EndFrame(CommandQueue * a_pQueue)
	{
		// 現フレームのフェンス値を進める
		m_currentFenceValue++;
		m_frameResources[m_cpuFrameIndex].fenceValue = m_currentFenceValue;

		// キューにシグナルを送る
		a_pQueue->Signal(m_cpFence.Get(), m_currentFenceValue);
	}

	void FrameManager::WaitForFrame()
	{
		// 次のフレームの描画準備がまだであれば待機する
		if (m_cpFence->GetCompletedValue() < m_frameResources[m_cpuFrameIndex].fenceValue)
		{
			// 完了時にイベントを設定
			HRESULT _hr = m_cpFence->SetEventOnCompletion(m_frameResources[m_cpuFrameIndex].fenceValue, m_fenceEvent);
			if (FAILED(_hr))
			{
				ENGINE_ERRLOG(SUCCEEDED(_hr), "フェンスイベントエラー HRESULT:%08X", _hr);
				return;
			}

			// 待機処理
			if (WAIT_OBJECT_0 != WaitForSingleObjectEx(m_fenceEvent, INFINITE, FALSE))
			{
				ENGINE_ERRLOG(false, "待機処理エラー");
				return;
			}
		}
	}

	void FrameManager::WaitForAll()
	{
		// 全フレームの中で最大のフェンス値を待つ
		UINT64 _maxFenceValue = 0;
		for (const auto& _res : m_frameResources)
		{
			_maxFenceValue = std::max(_maxFenceValue, _res.fenceValue);
		}

		// すでに完了していれば即リターン
		if (m_cpFence->GetCompletedValue() >= _maxFenceValue) return;

		m_cpFence->SetEventOnCompletion(_maxFenceValue, m_fenceEvent);
		WaitForSingleObjectEx(m_fenceEvent, INFINITE, FALSE);
	}
	bool FrameManager::IsFrameComplete(UINT a_frameIndex) const
	{
		return m_cpFence->GetCompletedValue() >= m_frameResources[a_frameIndex].fenceValue;
	}
	UINT64 FrameManager::GetFrameFenceValue(UINT a_frameIndex) const
	{
		return m_frameResources[a_frameIndex].fenceValue;
	}
	UINT FrameManager::GetCPUFrameIndex() const
	{
		return m_cpuFrameIndex;
	}
	ID3D12CommandAllocator* FrameManager::GetCurrentAllocator() const
	{
		return m_frameResources[m_cpuFrameIndex].cpAllocator.Get();
	}
	UINT64 FrameManager::GetCurrentFenceValue() const
	{
		return m_currentFenceValue;
	}
	UINT64 FrameManager::GetCompletedFenceValue() const
	{
		return m_cpFence->GetCompletedValue();
	}
	UINT64 FrameManager::GetNextFenceValue() const
	{
		// EndFrameで m_currentFenceValue をインクリメントしてからシグナルするため、
		// 記録中フレームの作業が完了する値は現在値の次になる
		return m_currentFenceValue + 1;
	}
}