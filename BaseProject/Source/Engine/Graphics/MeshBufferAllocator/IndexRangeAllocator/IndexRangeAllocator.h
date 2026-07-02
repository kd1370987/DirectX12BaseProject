#pragma once
namespace Engine::Graphics
{
	/// <summary>
	/// バッファと１対１で持つアロケータークラス
	/// </summary>
	class IndexRangeAllocator
	{
	public:

		/// <summary>
		/// 初期化 : 全体で一つのブロックを作る
		/// </summary>
		/// <param name="a_maxCount">最大領域数</param>
		void Init(uint32_t a_maxCount);

		/// <summary>
		/// 指定されたサイズの領域を確保する
		/// </summary>
		/// <param name="a_count">サイズ</param>
		/// <returns>割り当てられた領域</returns>
		IndexRangeHandle AllocateRange(uint32_t a_count);

		/// <summary>
		/// 指定した領域の解放 : 現在のフレームでは解放しずにフェンス値によって遅延開放される
		/// </summary>
		/// <param name="a_handle">指定した領域</param>
		/// <param name="a_currentFenceValue">現在のフェンス値</param>
		void FreeRange(const IndexRangeHandle& a_handle, uint64_t a_currentFenceValue);

		// 毎フレームの頭で呼ぶ：GPUが使い終わった領域をフリーリストに戻す
		
		/// <summary>
		/// 毎フレーム最初に呼ぶ : GPUが使い終わった領域をフリーリストに戻す
		/// メッシュバッファアロケータ側のバッファと同期する必要あり
		/// </summary>
		/// <param name="a_completedFenceValue">現在のフェンス値</param>
		void UpdateFrees(uint64_t a_completedFenceValue);

	private:

		/// <summary>
		/// 空いている領域を集めて塊にする
		/// </summary>
		void Merge();

	private:
		struct FreeBlock
		{
			uint32_t startIndex;
			uint32_t count;
		};

		// 遅延解放用の構造体
		struct PendingFree
		{
			FreeBlock block;
			uint64_t releaseFenceValue; // このフェンス値を越えたら再利用可能
		};

		std::vector<FreeBlock> m_freeBlocks;
		std::queue<PendingFree> m_pendingFrees;
		uint32_t m_currentAllocationId = 1;
		uint32_t m_maxCount = 0;
	};
}