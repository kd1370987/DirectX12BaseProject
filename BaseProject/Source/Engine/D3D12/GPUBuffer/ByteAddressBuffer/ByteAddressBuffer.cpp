#include "ByteAddressBuffer.h"
namespace Engine::D3D12
{
	bool Engine::D3D12::ByteAddressBuffer::Create(D3D12::Device* a_pDevice, const ByteAddressBufferDesc& a_desc)
	{
		// リソース作成
		DynamicBufferDesc _desc = {};
		_desc.elementNum = a_desc.count;
		_desc.strideSize = a_desc.strideSize;
		_desc.flags = D3D12_RESOURCE_FLAG_NONE;

		if (!DynamicBuffer::Create(a_pDevice, _desc))
		{
			ENGINE_ERRLOG(false,"バイトアドレスバッファの作成に失敗");
			return false;
		}


		// SRV作成
		D3D12_SHADER_RESOURCE_VIEW_DESC  _srv;
		_srv.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
		_srv.Buffer.FirstElement = 0;
		_srv.Buffer.NumElements = m_bufferSize / 4;
		_srv.Buffer.StructureByteStride = 0;
		_srv.Format = DXGI_FORMAT_R32_TYPELESS;
		_srv.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_RAW;
		DynamicBuffer::CreateSRVInternal(a_pDevice,_srv);

		return true;
	}

	const Handle<SRV>& Engine::D3D12::ByteAddressBuffer::GetSRVHandle() const
	{
		return m_srvHandle;
	}
}