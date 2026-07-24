#pragma once

namespace Engine::D3D12
{
	class RTVAllocator
	{
	public:

		// 生成
		bool Create(
			Engine::D3D12::DescriptorHeap<D3D12_DESCRIPTOR_HEAP_TYPE_RTV>* a_pHeap
		);

		// ビュー作成割り当て
		Engine::Resource::Handle<RTV> Allocate(
			ID3D12Resource* a_resource,
			D3D12_RENDER_TARGET_VIEW_DESC* a_pRTVDesc = nullptr
		);

		// ビュー消去
		void Remove(Engine::Resource::Handle<RTV> a_handle);

		// CPUハンドル取得
		D3D12_CPU_DESCRIPTOR_HANDLE GetCPU(const Engine::Resource::Handle<RTV>& a_handle) const;

	private:

		// 参照元ヒープ
		Engine::D3D12::DescriptorHeap<D3D12_DESCRIPTOR_HEAP_TYPE_RTV>* m_pHeap = nullptr;

		// 使用ハンドル行列
		std::vector<Engine::Resource::Generation> m_genVec = {};
		std::queue<Engine::Resource::Index> m_indexQueue;
	};
}