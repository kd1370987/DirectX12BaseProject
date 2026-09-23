#include "RenderDevice.h"

#include "../GraphicsDevice/GraphicsDevice.h"
#include "../CommandContext/CommandContext.h"
#include "../CommandPool/CommandPool.h"
#include "../FrameManager/FrameManager.h"
#include "../AsyncGPUManager/AsyncGPUManager.h"

namespace Engine::Graphics
{
	// unique_ptr の中身が完全型として見えるここで生成・破棄を定義する
	RenderDevice::RenderDevice() = default;
	RenderDevice::~RenderDevice() = default;

	bool RenderDevice::Init(bool a_isDebug)
	{
		m_upGraphicsDevice = std::make_unique<GraphicsDevice>();
		m_upGraphicsDevice->Create(a_isDebug);

		auto* _pDevice = m_upGraphicsDevice->RefDevice();
		if (!_pDevice)
		{
			ENGINE_ERRLOG(false, "デバイスの作成に失敗しました");
			return false;
		}

		// コマンドキュー(描画・コピー・コンピュート)
		m_upCommandContext = std::make_unique<CommandContext>();
		m_upCommandContext->Init(_pDevice);

		// 非同期転送の完了監視
		m_upAsyncGPUManager = std::make_unique<AsyncGPUManager>();
		m_upAsyncGPUManager->Init();

		// フレーム同期
		m_upFrameManager = std::make_unique<FrameManager>();
		m_upFrameManager->Init(_pDevice);

		ENGINE_LOG("デバイスとコマンドキューを作成");
		return true;
	}

	void RenderDevice::Release()
	{
		// GPUの完了を待ってからフレーム同期を片付ける
		if (m_upFrameManager)
		{
			m_upFrameManager->Release();
			m_upFrameManager.reset();
		}

		// 非同期転送の監視スレッドを止める
		if (m_upAsyncGPUManager)
		{
			m_upAsyncGPUManager->Release();
			m_upAsyncGPUManager.reset();
		}

		// コマンドキューとコマンドリスト(各プールがキューを空にしてから手放す)
		if (m_upCommandContext)
		{
			m_upCommandContext->RefDirectPool()->Release();
			m_upCommandContext->RefCopyPool()->Release();
			m_upCommandContext->RefComputePool()->Release();
			m_upCommandContext.reset();
		}

		if (!m_upGraphicsDevice) return;

		// 最後にデバイス。残っているオブジェクトはここでリークとして報告される
		m_upGraphicsDevice->Release();
		m_upGraphicsDevice.reset();
	}

	D3D12::Device* RenderDevice::RefDevice()
	{
		return m_upGraphicsDevice ? m_upGraphicsDevice->RefDevice() : nullptr;
	}

	//==========================================================================================
	//
	// コマンドキュー・フレーム同期
	//
	//==========================================================================================
	D3D12::GraphicsCommandList* RenderDevice::AcquireDirectCommandList()
	{
		// このフレームのアロケーターで記録を始める
		return m_upCommandContext->RefDirectPool()->AcquireList(
			RefDevice(),
			m_upFrameManager->GetCurrentAllocator()
		);
	}

	void RenderDevice::SubmitDirectCommandList(D3D12::GraphicsCommandList* a_pCmdList)
	{
		m_upCommandContext->RefDirectPool()->SubmitList(a_pCmdList);
	}

	void RenderDevice::ExecuteImmediate(D3D12::GraphicsCommandList* a_pCmdList)
	{
		m_upCommandContext->RefDirectPool()->ExecuteImmediate(a_pCmdList);
	}

	D3D12::CommandQueue* RenderDevice::RefDirectCommandQueue()
	{
		return m_upCommandContext ? m_upCommandContext->RefDirectPool()->GetCommandQueue() : nullptr;
	}

	UINT RenderDevice::GetCurrentFrameIndex() const
	{
		return m_upFrameManager ? m_upFrameManager->GetCPUFrameIndex() : 0;
	}

	void RenderDevice::WaitForFrame()
	{
		m_upFrameManager->WaitForAll();
	}

	void RenderDevice::WaitForGPUIdle()
	{
		// フレームのフェンスは Present より前に打たれているので、それを待っても
		// Present は終わっていない。キューごとに新しくシグナルを打って待つ
		m_upCommandContext->RefDirectPool()->WaitIdle();
		m_upCommandContext->RefCopyPool()->WaitIdle();
		m_upCommandContext->RefComputePool()->WaitIdle();
	}

	UINT64 RenderDevice::GetCurrentFenceValue() const
	{
		return m_upFrameManager->GetCurrentFenceValue();
	}
	UINT64 RenderDevice::GetCompletedFenceValue() const
	{
		return m_upFrameManager->GetCompletedFenceValue();
	}
	UINT64 RenderDevice::GetNextFenceValue() const
	{
		return m_upFrameManager->GetNextFenceValue();
	}

	//==========================================================================================
	//
	// 非同期転送
	//
	//==========================================================================================
	void RenderDevice::ExecuteAsyncCopy(std::function<void(D3D12::GraphicsCommandList*)> a_recordCmds, std::function<void()> a_onComplete)
	{
		auto* _pDevice = RefDevice();
		auto* _pCopyPool = m_upCommandContext->RefCopyPool();

		// 非同期マネージャーからアロケーターをもらう
		auto* _allocator = m_upAsyncGPUManager->AcquireAllocator(_pDevice, AsyncCommandType::Copy);

		// コピー用のコマンドプールからリストをもらう (内部で_allocatorを使ってResetされる)
		D3D12::GraphicsCommandList* _cmdList = _pCopyPool->AcquireList(_pDevice, _allocator);

		// 外部から渡された「コマンドを積む処理」を実行
		if (a_recordCmds)
		{
			a_recordCmds(_cmdList);
		}

		// プールにリストを返し、即座に実行する(戻り値のフェンス値を受け取る)
		_pCopyPool->SubmitList(_cmdList);
		UINT64 _fenceValue = _pCopyPool->ExecutePendingLists();

		// 非同期マネージャーに監視を依頼する（キュー管理と寿命監視の連携）
		m_upAsyncGPUManager->RegisterTask(
			AsyncCommandType::Copy,
			_allocator,
			_pCopyPool->GetFence(),
			_fenceValue,
			a_onComplete
		);
	}

	AsyncBuildBatch RenderDevice::BeginAsyncBuildBatch(bool a_useCopy, bool a_useCompute)
	{
		auto* _pDevice = RefDevice();
		AsyncBuildBatch _batch = {};

		// コピー用
		if (a_useCopy)
		{
			_batch.pCopyAllocator = m_upAsyncGPUManager->AcquireAllocator(_pDevice, AsyncCommandType::Copy);
			_batch.pCopyCmdList = m_upCommandContext->RefCopyPool()->AcquireList(_pDevice, _batch.pCopyAllocator);
		}

		// コンピュート用
		if (a_useCompute)
		{
			_batch.pComputeAllocator = m_upAsyncGPUManager->AcquireAllocator(_pDevice, AsyncCommandType::Compute);
			_batch.pComputeCmdList = m_upCommandContext->RefComputePool()->AcquireList(_pDevice, _batch.pComputeAllocator);
		}

		return _batch;
	}

	void RenderDevice::EndAsyncBuildBatch(AsyncBuildBatch& a_batch, std::function<void()> a_onComplete)
	{
		auto* _pCopyPool = m_upCommandContext->RefCopyPool();
		auto* _pComputePool = m_upCommandContext->RefComputePool();

		const bool _hasCopy = (a_batch.pCopyCmdList != nullptr);
		const bool _hasCompute = (a_batch.pComputeCmdList != nullptr);

		// ---- コピーの実行 ----
		UINT64 _copyFenceValue = 0;
		if (_hasCopy)
		{
			_pCopyPool->SubmitList(a_batch.pCopyCmdList);
			_copyFenceValue = _pCopyPool->ExecutePendingLists();
		}

		// ---- コンピュートの実行 ----
		if (_hasCompute)
		{
			// BLASはコピーで転送したメガバッファを直接読むため、
			// コピーの完了をコンピュートキュー側で待たせる
			if (_hasCopy)
			{
				_pComputePool->GetCommandQueue()->Wait(_pCopyPool->GetFence(), _copyFenceValue);
			}

			_pComputePool->SubmitList(a_batch.pComputeCmdList);
			UINT64 _computeFenceValue = _pComputePool->ExecutePendingLists();

			// 完了通知はGPU処理の最後になるコンピュート側に載せる
			m_upAsyncGPUManager->RegisterTask(
				AsyncCommandType::Compute,
				a_batch.pComputeAllocator,
				_pComputePool->GetFence(),
				_computeFenceValue,
				a_onComplete
			);

			// コンピュート側で消化したので、コピー側では呼ばない
			a_onComplete = nullptr;
		}

		// ---- コピー側のアロケーター返却と中間バッファの解放 ----
		if (_hasCopy)
		{
			m_upAsyncGPUManager->RegisterTask(
				AsyncCommandType::Copy,
				a_batch.pCopyAllocator,
				_pCopyPool->GetFence(),
				_copyFenceValue,
				[_keepAlive = std::move(a_batch.keepAliveResources), _onComplete = std::move(a_onComplete)]()
				{
					// _keepAlive のデストラクタで中間のUploadバッファが解放される
					if (_onComplete) _onComplete();
				}
			);
		}

		// 使い終わったバッチを空にする
		a_batch = {};
	}

	//==========================================================================================
	//
	// フレーム
	//
	//==========================================================================================
	void RenderDevice::BeginFrame()
	{
		m_upFrameManager->BeginFrame();
	}

	void RenderDevice::EndFrame()
	{
		auto* _pDirectPool = m_upCommandContext->RefDirectPool();

		// 積んだリストを流して、フレーム終了のシグナルを打つ
		_pDirectPool->ExecutePendingLists();
		m_upFrameManager->EndFrame(_pDirectPool->GetCommandQueue());
	}
}
