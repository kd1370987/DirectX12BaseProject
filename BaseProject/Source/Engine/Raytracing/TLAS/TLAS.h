#pragma once

namespace Engine::Raytracing
{

	class TLAS
	{
	public:

		// 作成
		void Create(
			D3D12::Device* a_pDevice,
			D3D12::GraphicsCommandList* a_pCmdList,
			UINT a_maxInstanceNum
		);

		// 解放
		void Release();

		// 更新
		void Update(D3D12::GraphicsCommandList* a_pCmdList,const std::vector<Instance>& a_instanceVec);

		D3D12_GPU_VIRTUAL_ADDRESS GetGPUAddress() const
		{
			return m_cpResource->GetGPUVirtualAddress();
		}

		D3D12_GPU_DESCRIPTOR_HANDLE GetGPUHandle();

		void SetHitGroupNum(uint32_t a_num)
		{
			m_hitGroupNum = a_num;
		}

	private:

		void CreateBuffer(
			D3D12::Device* a_pDevice,
			D3D12::GraphicsCommandList* a_pCmdList,
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
		uint32_t m_instanceCount = 0;
		const uint32_t m_maxInstanceCount = 1000;
		D3D12_RAYTRACING_INSTANCE_DESC* m_pInstanceDesc = nullptr;		// マップしておく

		// SRVハンドル
		Engine::Handle<D3D12::SRV> m_srvHandle = {};

		// ヒットグループ数
		uint32_t m_hitGroupNum = 0;
	};
}