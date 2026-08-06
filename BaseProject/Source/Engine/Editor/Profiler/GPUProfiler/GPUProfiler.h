#pragma once
namespace Engine::Editor
{
	struct Timer;

	/// <summary>
	/// GPUの実行時間などを図るためのもの
	/// </summary>
	class GPUProfiler
	{
	public:

		//-------------------------------------------------------------------------------------
		// 全体設定
		//-------------------------------------------------------------------------------------
		// 初期化
		void Init();
		void Releasse();

		// 更新
		void BeginFrame();
		void EndFrame(
			D3D12::CommandQueue* a_pCmdQueue,
			std::unordered_map<std::string, Timer>& a_timers
		);

		//-------------------------------------------------------------------------------------
		// 個別設定
		//-------------------------------------------------------------------------------------
		void Start(D3D12::GraphicsCommandList* a_pCmdList, Timer& a_timer);
		void End(D3D12::GraphicsCommandList* a_pCmdList, Timer& a_timer);

	private:

		// 記録・読み出しに使うフレームスロットを取得する
		UINT CurrentFrameIndex() const;

	private:
		ComPtr<ID3D12QueryHeap> m_upQueryHeap;

		// リードバックバッファはフレームごとに持つ
		//
		// 1枚しかないと、待機で完了が保証されたフレームの結果を読んでいる最中に
		// 飛行中の別フレームが同じ領域へResolveしてきて値が混ざる。
		D3D12::GPUBuffer m_readBackBuffers[CPU_FRAME_COUNT];
		uint64_t* m_mapData[CPU_FRAME_COUNT] = {};

		uint32_t m_nextQuery = 0;

		UINT m_maxQueries = 256;		// 1フレーム内で使用できるクエリ最大数

		// クエリ0番は「GPU計測なし」を表す予約枠として使うので、割り当ては1番から始める
		static constexpr uint32_t INVALID_QUERY = 0;
	};
}
