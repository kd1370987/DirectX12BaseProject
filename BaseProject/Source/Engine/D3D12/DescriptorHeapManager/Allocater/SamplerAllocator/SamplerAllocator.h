#pragma once

namespace Engine::D3D12
{
	class SamplerAllocator
	{
	public:

		// 生成
		bool Create(
			Engine::D3D12::DescriptorHeap<D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER>* a_pHeap
		);

		// ビュー作成割り当て
		Engine::Resource::Handle<SAMPLER> Allocate(
			ID3D12Device* a_pDevice,
			const D3D12_SAMPLER_DESC& a_desc
		);

		// ビュー消去
		void Remove(Engine::Resource::Handle<SAMPLER> a_handle);

		// ハンドル取得
		D3D12_CPU_DESCRIPTOR_HANDLE GetCPU(const Engine::Resource::Handle<SAMPLER>& a_handle) const;
		D3D12_GPU_DESCRIPTOR_HANDLE GetGPU(const Engine::Resource::Handle<SAMPLER>& a_handle) const;

	private:

		// 参照元ヒープ
		Engine::D3D12::DescriptorHeap<D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER>* m_pHeap = nullptr;

		// 使用ハンドル行列
		std::vector<Engine::Resource::Generation> m_genVec = {};
		std::queue<Engine::Resource::Index> m_indexQueue;
	};
}