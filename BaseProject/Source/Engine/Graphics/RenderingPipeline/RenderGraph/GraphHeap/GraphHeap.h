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

		void Create(D3D12::Device* a_pDevice,UINT64 a_maxWidth,UINT a_maxHeight);

	private:

		ComPtr<ID3D12Heap> m_cpHeap = nullptr;
		UINT64 m_width = 0;
		UINT m_height = 0;

		
		UINT m_currentOffset = 0;

	};
}