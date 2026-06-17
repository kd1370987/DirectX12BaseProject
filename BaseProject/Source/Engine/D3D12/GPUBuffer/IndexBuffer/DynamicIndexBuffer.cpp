#include "DynamicIndexBuffer.h"

namespace Engine::D3D12
{
	bool DynamicIndexBuffer::Create(ID3D12Device* a_pDebice, const IndexBufferDesc& a_desc)
	{
		// リソース作成
		DynamicBufferDesc _desc = {};
		_desc.elementNum = a_desc.count;
		_desc.strideSize = (a_desc.format == DXGI_FORMAT_R16_UINT) ? 2 : 4;
		_desc.flags = D3D12_RESOURCE_FLAG_NONE;

		if (!DynamicBuffer::Create(a_pDebice, _desc))
		{
			assert(0 && "インデックスバッファ作成時にリソース作成失敗");
			return false;
		}
		

		// インデックスバッファビュー作成
		m_view = {};
		m_view.BufferLocation = GetGPUVirtualAddress();
		m_view.SizeInBytes = static_cast<UINT>(GetBufferSize());
		m_view.Format = a_desc.format;

		// 初期化データ
		if (a_desc.pData)
		{
			UpdateData(a_desc.pData, GetBufferSize());
		}

		return true;
	}
	void DynamicIndexBuffer::CreateSRV(ID3D12Device* a_pDevice)
	{
		DynamicBuffer::CreateSRVInternal(a_pDevice);
	}

	const D3D12_INDEX_BUFFER_VIEW& DynamicIndexBuffer::GetView() const
	{
		return m_view;
	}
	const Handle<SRV>& DynamicIndexBuffer::GetSRVHandle() const
	{
		return m_srvHandle;
	}
}