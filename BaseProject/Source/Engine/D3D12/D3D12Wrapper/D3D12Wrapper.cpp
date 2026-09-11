#include "D3D12Wrapper.h"

#include "../Command/CommandContext/CommandContext.h"
#include "../Command/CommandPool/CommandPool.h"

#include "../FrameManager/FrameManager.h"

#include "../AsyncGPUManager/AsyncGPUManager.h"

namespace Engine::D3D12
{
	void D3D12Wrapper::Init(Device* a_pDevice)
	{
		// デバイスは借りるだけ。作るのも捨てるのも GraphicsEngine
		m_pDevice = a_pDevice;
		if (!m_pDevice)
		{
			ENGINE_ERRLOG(false, "D3D12Wrapper の初期化にデバイスが渡されていません");
			return;
		}

		// コマンドコンテキスト作成
		CreateCommandContext();

		// 非同期マネージャー作成
		CreateAsyncGPUManager();

		// フレームマネージャー作成
		CreateFrameManager();

		ENGINE_LOG("D3D12Wrapper作成");
	}

	void D3D12Wrapper::Release()
	{
		// GPU待機処理
		m_upFrameManager->Release();

		// 非同期マネージャー解放
		m_upAsyncGPUManager->Release();

		// コマンドリスト解放
		m_upCommandContext->RefDirectPool()->Release();
		m_upCommandContext->RefCopyPool()->Release();
		m_upCommandContext->RefComputePool()->Release();

		// デバイスは借り物なので手放すだけ。
		// リークレポートと解放は GraphicsEngine::ReleaseDevice() が最後に行う
		m_pDevice = nullptr;
	}

	//==================================================================================
	//
	// 描画開始・描画終了
	//
	//==================================================================================
	void D3D12Wrapper::BeginFrame()
	{
		// フレームインデックス更新 : GPU待機
		m_upFrameManager->BeginFrame();
	}
	void D3D12Wrapper::EndFrame()
	{
		// コマンドリストの実行
		m_upCommandContext->RefDirectPool()->ExecutePendingLists();

		// フレーム終了シグナル
		m_upFrameManager->EndFrame(m_upCommandContext->RefDirectPool()->GetCommandQueue());
	}

	void D3D12Wrapper::ExecuteAsyncCompute(std::function<void(GraphicsCommandList*)> a_recordCmds, std::function<void()> a_onComplete)
	{
		// 処理の流れはCopyと全く同じで、Compute用のプールとタイプを指定します
		auto* _allocator = m_upAsyncGPUManager->AcquireAllocator(m_pDevice, AsyncCommandType::Compute);
		GraphicsCommandList* _cmdList = m_upCommandContext->RefComputePool()->AcquireList(m_pDevice, _allocator);

		if (a_recordCmds) {
			a_recordCmds(_cmdList);
		}

		m_upCommandContext->RefComputePool()->SubmitList(_cmdList);
		UINT64 _fenceValue = m_upCommandContext->RefComputePool()->ExecutePendingLists();

		m_upAsyncGPUManager->RegisterTask(
			AsyncCommandType::Compute,
			_allocator,
			m_upCommandContext->RefComputePool()->GetFence(),
			_fenceValue,
			a_onComplete
		);
	}

	AsyncBuildBatch D3D12Wrapper::BeginAsyncBuildBatch(bool a_useCopy, bool a_useCompute)
	{
		AsyncBuildBatch _batch = {};

		// コピー用
		if (a_useCopy)
		{
			_batch.pCopyAllocator = m_upAsyncGPUManager->AcquireAllocator(m_pDevice, AsyncCommandType::Copy);
			_batch.pCopyCmdList = m_upCommandContext->RefCopyPool()->AcquireList(m_pDevice, _batch.pCopyAllocator);
		}

		// コンピュート用
		if (a_useCompute)
		{
			_batch.pComputeAllocator = m_upAsyncGPUManager->AcquireAllocator(m_pDevice, AsyncCommandType::Compute);
			_batch.pComputeCmdList = m_upCommandContext->RefComputePool()->AcquireList(m_pDevice, _batch.pComputeAllocator);
		}

		return _batch;
	}

	void D3D12Wrapper::EndAsyncBuildBatch(AsyncBuildBatch& a_batch, std::function<void()> a_onComplete)
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

	void D3D12Wrapper::WaitForAsyncTasks()
	{
	}

	void D3D12Wrapper::WaitForFrame()
	{
		m_upFrameManager->WaitForAll();
	}

	void D3D12Wrapper::WaitForGPUIdle()
	{
		// フレームのフェンスは Present より前に打たれているので、それを待っても
		// Present は終わっていない。キューごとに新しくシグナルを打って待つ
		m_upCommandContext->RefDirectPool()->WaitIdle();
		m_upCommandContext->RefCopyPool()->WaitIdle();
		m_upCommandContext->RefComputePool()->WaitIdle();
	}

	//==================================================================================
	//
	// ゲッター
	//
	//==================================================================================
	// デバイス取得
	Device* D3D12Wrapper::GetDevice()
	{
		return m_pDevice;
	}

	// 現在のCPUフレーム番号取得
	UINT D3D12Wrapper::CurrentCPUFrameIndex()
	{
		return m_upFrameManager->GetCPUFrameIndex();
	}

	// コマンドキュー取得
	ID3D12CommandQueue* D3D12Wrapper::GetCommandQueue()
	{
		return m_upCommandContext->RefDirectPool()->GetCommandQueue();
	}
	ID3D12CommandQueue* D3D12Wrapper::GetCopyCommandQueue()
	{
		return m_upCommandContext->RefCopyPool()->GetCommandQueue();
	}
	ID3D12CommandQueue* D3D12Wrapper::GetComputeCommandQueue()
	{
		return m_upCommandContext->RefComputePool()->GetCommandQueue();
	}

	// コマンドリスト取得
	GraphicsCommandList* D3D12Wrapper::GetDirectCommandList()
	{
		return m_upCommandContext->RefDirectPool()->AcquireList(
			m_pDevice,
			m_upFrameManager->GetCurrentAllocator()
		);
	}
	UINT64 D3D12Wrapper::GetCurrentFenceValue()
	{
		return m_upFrameManager->GetCurrentFenceValue();
	}
	UINT64 D3D12Wrapper::GetCompletedFenceValue()
	{
		return m_upFrameManager->GetCompletedFenceValue();
	}
	UINT64 D3D12Wrapper::GetNextFenceValue()
	{
		return m_upFrameManager->GetNextFenceValue();
	}

	//==================================================================================
	//
	// 管理クラス作成
	//
	//==================================================================================
	void D3D12Wrapper::CreateCommandContext()
	{
		m_upCommandContext = std::make_unique<CommandContext>();
		m_upCommandContext->Init(m_pDevice);
	}

	void D3D12Wrapper::CreateFrameManager()
	{
		m_upFrameManager = std::make_unique<FrameManager>();
		m_upFrameManager->Init(m_pDevice);
	}

	void D3D12Wrapper::CreateAsyncGPUManager()
	{
		m_upAsyncGPUManager = std::make_unique<AsyncGPUManager>();
		m_upAsyncGPUManager->Init();
	}

	void D3D12Wrapper::CloseAndExecuteComdLists(GraphicsCommandList* a_pCmdList)
	{
		// コマンドリストを実行
		m_upCommandContext->RefDirectPool()->ExecuteImmediate(a_pCmdList);
	}

	void D3D12Wrapper::SubmitDirectCommandList(GraphicsCommandList* a_pCmdList)
	{
		m_upCommandContext->RefDirectPool()->SubmitList(a_pCmdList);
	}

	void D3D12Wrapper::SubmitCopyCommandList(GraphicsCommandList * a_pCmdList)
	{
		m_upCommandContext->RefCopyPool()->SubmitList(a_pCmdList);
	}

	void D3D12Wrapper::SubmitComputeCommandList(GraphicsCommandList * a_pCmdList)
	{
		m_upCommandContext->RefComputePool()->SubmitList(a_pCmdList);
	}

	void D3D12Wrapper::ExecuteDirectCommandList()
	{
		// 実行待ちリストを処理
		m_upCommandContext->RefDirectPool()->ExecutePendingLists();

		// 終了待機
		m_upFrameManager->WaitForFrame();
	}

	void D3D12Wrapper::ExecuteCopyCommandList()
	{

	}

	void D3D12Wrapper::ExecuteComputeCommandList()
	{

	}

	void D3D12Wrapper::ExecuteAsyncCopy(std::function<void(GraphicsCommandList*)> a_recordCmds, std::function<void()> a_onComplete)
	{
		// 非同期マネージャーからアロケーターをもらう
		auto* _allocator = m_upAsyncGPUManager->AcquireAllocator(m_pDevice, AsyncCommandType::Copy);

		// コピー用のコマンドプールからリストをもらう (内部で_allocatorを使ってResetされる)
		GraphicsCommandList* _cmdList = m_upCommandContext->RefCopyPool()->AcquireList(m_pDevice, _allocator);

		// 外部から渡された「コマンドを積む処理」を実行
		if (a_recordCmds) {
			a_recordCmds(_cmdList);
		}

		// コンテキストにリストを返し、一括実行 (戻り値のフェンス値を受け取る)
		m_upCommandContext->RefCopyPool()->SubmitList(_cmdList);

		// ※注意: もし他にも同時に積みたいパスがあれば、ExecutePendingListsの呼び出しは遅らせてもOKです。
		// 今回は即座に裏スレッドへ投げる想定でここでExecuteします。
		UINT64 _fenceValue = m_upCommandContext->RefCopyPool()->ExecutePendingLists();

		// 非同期マネージャーに監視を依頼する（キュー管理と寿命監視の連携）
		m_upAsyncGPUManager->RegisterTask(
			AsyncCommandType::Copy,
			_allocator,
			m_upCommandContext->RefCopyPool()->GetFence(),
			_fenceValue,
			a_onComplete
		);
	}


	D3D12Wrapper::D3D12Wrapper()
	{}

	D3D12Wrapper::~D3D12Wrapper()
	{}
}
