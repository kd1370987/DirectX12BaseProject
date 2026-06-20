#pragma once

namespace Engine::D3D12
{
	/// <summary>
	/// フレームごとのリソース
	/// </summary>
	struct FrameResource
	{
		ComPtr<ID3D12CommandAllocator> cpAllocator = nullptr;
		UINT64                         fenceValue = 0;
	};

	/// <summary>
	/// フレームインデックス・フェンス・GPU同期を一元管理
	/// </summary>
	class FrameManager
	{
	public:

		/// <summary>
		/// 初期化
		/// </summary>
		void Init(Device* a_pDevice);

		/// <summary>
		/// 解放 : WaitForAllしてから各リソースを解放
		/// </summary>
		void Release();

		/// <summary>
		/// フレーム開始 : 前フレームのGPU完了を待ってアロケーターをリセット
		/// </summary>
		void BeginFrame();

		/// <summary>
		/// フレーム終了 : フェンスにシグナルを送る
		/// </summary>
		void EndFrame(CommandQueue* a_pQueue);

		/// <summary>
		/// GPU完了を待つ（フレーム指定）
		/// </summary>
		void WaitForFrame();

		/// <summary>
		/// GPU完了を待つ（全フレーム）
		/// </summary>
		void WaitForAll();

		/// <summary>
		/// 指定フレームが完了しているか（ノンブロッキング）
		/// ジョブの投入判定などに使う
		/// CPUを止めずに処理が終わったかの確認
		/// </summary>
		bool IsFrameComplete(UINT a_frameIndex) const;

		/// <summary>
		/// 指定フレームのフェンス値を取得
		/// CommandPoolの遅延解放などに使う
		/// いつGPUの処理が終わるかの目印をつける
		/// </summary>
		UINT64 GetFrameFenceValue(UINT a_frameIndex) const;

		// ---- アクセサ ----
		UINT                    GetCPUFrameIndex()     const;
		ID3D12CommandAllocator* GetCurrentAllocator()  const;
		UINT64                  GetCurrentFenceValue() const;

	private:

		// GPU同期待ち用フェンス
		ComPtr<Fence>   m_cpFence = nullptr;		// フェンス本体
		HANDLE          m_fenceEvent = nullptr;		// イベント
		UINT64          m_currentFenceValue = 0;	// 現在の値

		// フレーム
		UINT            m_cpuFrameIndex = 0;						// 現在のフレームインデックス
		FrameResource   m_frameResources[CPU_FRAME_COUNT] = {};		// フレームごとのデータ
	};	
}
