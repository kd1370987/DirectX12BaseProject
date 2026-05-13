#pragma once

#include "../DynamicBuffer/DynamicBuffer.h"

namespace Engine::D3D12
{
	struct IndexBufferDesc
	{
		const void* pData = nullptr;
		uint32_t count  = 0;
		DXGI_FORMAT format = DXGI_FORMAT_R32_UINT;
	};

	class DynamicIndexBuffer : public DynamicBuffer
	{
	public:

		DynamicIndexBuffer() = default;
		~DynamicIndexBuffer() override = default;

		// 作成
		bool Create(ID3D12Device* a_pDevice,const IndexBufferDesc& a_desc);

		// SRV作成
		void CreateSRV(ID3D12Device* a_pDevice);

		// アクセサ
		const D3D12_INDEX_BUFFER_VIEW& GetView() const;
		const Resource::Handle<SRV>& GetSRVHandle() const;

	private:
		D3D12_INDEX_BUFFER_VIEW m_view = {};
	};
}

