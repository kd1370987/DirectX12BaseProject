#pragma once

namespace Engine::Raytracing
{

	class TLAS
	{
	public:

		void Create(
			const std::vector<Instance>& a_instanceVec
		);

		void Build(
			ID3D12GraphicsCommandList4* a_pCmdList
		);

		D3D12_GPU_VIRTUAL_ADDRESS GetGPUAddress() const
		{
			return m_cpResource->GetGPUVirtualAddress();
		}

	private:

		void CreateBuffer(
			ID3D12Device5* a_pDevice,
			ComPtr<ID3D12Resource>& a_cpRes,
			uint64_t a_size,
			D3D12_RESOURCE_FLAGS a_flags,
			D3D12_RESOURCE_STATES a_initState,
			const D3D12_HEAP_PROPERTIES& a_heapProps
		);

	private:

		// TLAS本体
		ComPtr<ID3D12Resource> m_cpResource = nullptr;
		uint64_t m_resultSize = 0;

		// scratchBuffer
		ComPtr<ID3D12Resource> m_cpScratch = nullptr;
		uint64_t m_scratchSize = 0;

		// インスタンスバッファ
		ComPtr<ID3D12Resource> m_cpInstanceBuffer = nullptr;
		uint32_t m_instnaceCount = 0;
	};
}