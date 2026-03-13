#pragma once

namespace Engine::D3D12
{
	class UAVAllocator
	{
	public:

		// 生成
		bool Create(
			Engine::D3D12::DescriptorHeap<D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV>* a_pHeap,
			UINT a_UAVStart,
			UINT a_UAVCount
		);


		// ビューを配置
		std::vector<Engine::Resource::Handle<UAV>> Allocate(std::vector<UAVViewInit> a_resourceVec);

		// ビュー消去
		void Remove(Engine::Resource::Handle<UAV> a_handle);

		// CPUハンドル取得
		D3D12_CPU_DESCRIPTOR_HANDLE GetCPU(const Engine::Resource::Handle<UAV>& a_handle) const;
		D3D12_GPU_DESCRIPTOR_HANDLE GetGPU(const Engine::Resource::Handle<UAV>& a_handle) const;

	private:

		bool Check(const Engine::Resource::Handle<UAV>& a_handle) const;

	private:

		// 参照元ヒープ
		Engine::D3D12::DescriptorHeap<D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV>* m_pHeap = nullptr;

		std::vector<Engine::Resource::Generation> m_genVec = {};		// 世代配列
		std::queue<Engine::Resource::Index> m_indexQueue = {};			// 使用ハンドル行列

		// メモリ使用領域計算
		FreeRange m_UAVRangeList = {};

		// UAVの開始位置
		UINT m_UAVStartIndex = 0;
	};
}