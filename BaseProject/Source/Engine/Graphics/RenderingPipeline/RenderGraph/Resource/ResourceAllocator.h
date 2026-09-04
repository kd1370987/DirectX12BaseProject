#pragma once
namespace Engine::Graphics::Pipeline
{
	class VirtualResource;

	/// <summary>
	/// 割り当てられたと仮定した際のリソース
	/// </summary>
	struct AllocationSlot
	{
		uint64_t offset = 0;					// ヒープ内でこのスロットの開始位置
		uint64_t allocationSize = 0;
		uint64_t allocationAlignment = 0;

		// このスロットを最後に使用したVirtualResuorceの終了パス
		uint32_t lastPassIndex = 0;

		VirtualResource* pLastResource = nullptr;
	};

	/// <summary>
	/// エイリアシング計算、実際のリソースが割り当てられる際の制御
	/// </summary>
	class ResourceAllocator
	{
	public:

		/// <summary>
		/// ヒープサイズと各スロットのオフセットを決める
		/// </summary>
		/// <param name="a_virtualResources"></param>
		void CalcAllocation(std::vector<VirtualResource>& a_virtualResources);

		// ヒープの定義取得
		uint32_t GetMaxUageSlot() const { return m_maxUageSlot; }
		uint64_t GetMaxHeapSize() const { return m_maxHeapSize; }

		// 割り当ての結果。使い回しの様子を外から覗くのに使う
		const std::vector<AllocationSlot>& GetSlots() const { return m_slots; }

	private:

		std::vector<AllocationSlot> m_slots;
		uint32_t m_maxUageSlot = 0;
		uint64_t m_maxHeapSize = 0;
	};
}