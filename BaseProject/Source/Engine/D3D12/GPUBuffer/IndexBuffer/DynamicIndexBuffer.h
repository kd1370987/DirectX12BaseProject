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

		~DynamicIndexBuffer() override = default;

		NON_COPYABLE_MOVABLE(DynamicIndexBuffer);

		// 作成
		bool Create(D3D12::Device* a_pDevice,const IndexBufferDesc& a_desc);

		// アクセサ
		const D3D12_INDEX_BUFFER_VIEW& GetView() const;

	private:
		D3D12_INDEX_BUFFER_VIEW m_view = {};
	};
}

