#pragma once

namespace Engine::D3D12
{
	struct AllocationEntry
	{
		Engine::Resource::Index start = Engine::Resource::Limits::INVALID_INDEX;
		UINT count = 0;
		Engine::Resource::Generation gen = Engine::Resource::Limits::INVALID_GENERATION;
	};

	class SRVAllocator
	{
	public:

		// 生成
		bool Create(
			Engine::D3D12::DescriptorHeap<D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV>* a_pHeap,
			UINT a_srvStart,
			UINT a_srvCount
		);

		// ビュー作成割り当て
		Engine::Resource::HandleRange<SRV> Allocate(
			std::vector<SRVViewInit> a_resourceVec
		);

		// ビュー消去
		void Remove(Engine::Resource::Handle<SRV> a_handle);

		// CPUハンドル取得
		D3D12_CPU_DESCRIPTOR_HANDLE GetCPU(const Engine::Resource::HandleRange<SRV>& a_handle) const;
		D3D12_GPU_DESCRIPTOR_HANDLE GetGPU(const Engine::Resource::HandleRange<SRV>& a_handle) const;

	private:

		// 参照元ヒープ
		Engine::D3D12::DescriptorHeap<D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV>* m_pHeap = nullptr;

		// 使用ハンドル行列
		std::vector<AllocationEntry> m_entoryVec = {};
		std::queue<Engine::Resource::Index> m_indexQueue;

		// メモリ使用領域計算
		FreeRange m_srvRangeList = {};

		// SRVの開始位置
		UINT m_srvStartIndex = 0;
	};
}