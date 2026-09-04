#pragma once
namespace Engine::Graphics::Pipeline
{
	/// <summary>
	/// レンダーグラフに持たせるヒープ
	/// 
	/// リソースを発行する際にエイリアシングできるものはこのヒープに割り当てる
	/// </summary>
	class GraphHeap
	{
	public:

		GraphHeap() = default;
		~GraphHeap() = default;

		void Create(D3D12::Device* a_pDevice,UINT64 a_maxHeapSize);

		// 参照
		ID3D12Heap* RefHeap() { return m_cpHeap.Get(); }
		UINT64 GetMaxHeapSize() const { return m_maxHeapSize; }

	private:

		ComPtr<ID3D12Heap> m_cpHeap = nullptr;
		UINT64 m_maxHeapSize = 0;
	};
}