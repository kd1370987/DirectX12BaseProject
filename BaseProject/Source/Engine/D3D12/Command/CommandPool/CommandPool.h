#pragma once

namespace Engine::D3D12
{

	// 使用中のコマンドリストと完了フェンス値をペアで持つ
	struct InFlightList
	{
		ComPtr<GraphicsCommandList> cpList;
		UINT64 fenceValue;						// この値を超えたら返却
	};

	/// <summary>
	/// コマンドキューを持ち、そのキューと対応するコマンドリストのプールを持つクラス
	/// CPUの処理が止まることはないが、一つのコマンドリストはGPUの処理が終わるまでは使えない
	/// </summary>
	class CommandPool
	{
	public:

		CommandPool() = default;
		~CommandPool() { Release(); }

		/// <summary>
		/// 初期化 : コマンドキューの作成
		/// </summary>
		/// <param name="a_pDevice"></param>
		/// <param name="a_type"></param>
		void Init(Device* a_pDevice,D3D12_COMMAND_LIST_TYPE a_type);

		/// <summary>
		/// 解放処理
		/// </summary>
		void Release();

		/// <summary>
		/// コマンドリストの取得 : 内部のリセット処理が走る
		/// </summary>
		/// <returns>コマンドリストの生ポインタ</returns>
		GraphicsCommandList* AcquireList(Device* a_pDevice, ID3D12CommandAllocator* a_pAllocator);

		/// <summary>
		/// 記録が終わったリストを実行待ちへためる
		/// </summary>
		/// <param name="a_pList">記録が終わったリスト</param>
		void SubmitList(GraphicsCommandList* a_pList);

		/// <summary>
		/// 実行待ちリストを一括でキューへ投げて実行 : CPU待機時間が発生する
		/// </summary>
		UINT64 ExecutePendingLists();

		/// <summary>
		/// コマンドリストの即時実行 : GPU完了まで待つためCPUが止まる
		/// 初期化時などランタイム外でGPU操作が必要なときなどに使う
		/// </summary>
		void ExecuteImmediate(GraphicsCommandList* a_pList);

		// ---- アクセサ ----
		CommandQueue* GetCommandQueue() { return m_cpCmdQueue.Get(); }
		D3D12_COMMAND_LIST_TYPE GetQueueType() { return m_type; }
		Fence* GetFence() { return m_cpFence.Get(); }
	private:
		
		// コマンドキューとフェンス
		ComPtr<CommandQueue>	m_cpCmdQueue	= nullptr;	// コマンドキュー
		ComPtr<Fence>			m_cpFence		= nullptr;	// 使用終了待ち用フェンス
		UINT64					m_fenceValue	= 0;		// 現在のフェンス値

		// キューの種類
		D3D12_COMMAND_LIST_TYPE m_type = D3D12_COMMAND_LIST_TYPE_DIRECT;

		// コマンドリストプール
		std::vector<ComPtr<GraphicsCommandList>>	m_freeLists		= {};	// 使えるリスト
		std::vector<InFlightList>					m_inFlightLists = {};	// GPU実行中のリスト

		// 生ポインタからComPtrの追跡
		std::unordered_map<GraphicsCommandList*, ComPtr<GraphicsCommandList>> m_trackingMap = {};

		// 非同期用
		std::mutex m_mutex;

		// 実行待ちのコマンドリスト生ポインタ配列
		std::vector<GraphicsCommandList*> m_pendingLists;
	};
}