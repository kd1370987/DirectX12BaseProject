#pragma once

#include "../GPUBuffer.h"

namespace Engine::D3D12
{
	struct DynamicBufferDesc
	{
		size_t elementNum = 0;
		size_t strideSize = 0;
		D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE;
	};

	// 高頻度で内容が更新されるようなバッファの親クラス
	class DynamicBuffer : public GPUBuffer
	{
	public:

		virtual ~DynamicBuffer() override = default;
		NON_COPYABLE_MOVABLE(DynamicBuffer);

		// 作成
		bool Create(ID3D12Device* a_pDevice,const DynamicBufferDesc& a_desc);

		// 操作
		virtual void UpdateData(const void* a_data,size_t a_size);

	protected:
		// SRV作成関数
		void CreateSRVInternal(ID3D12Device* a_pDevice);

	protected:
		void* m_pMapData = nullptr;
		Resource::Handle<D3D12::SRV> m_srvHandle = {};
	};
}