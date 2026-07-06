#include "DynamicBuffer.h"
#include "../../DescriptorHeapManager/DescriptorHeapManager.h"

bool Engine::D3D12::DynamicBuffer::Create(D3D12::Device* a_pDevice, const DynamicBufferDesc& a_desc)
{
	// リソース作成
	GPUBufferDesc _desc = {};
	_desc.elementNum = a_desc.elementNum;
	_desc.strideSize = a_desc.strideSize;
	_desc.flags = a_desc.flags;
	_desc.heapType = D3D12_HEAP_TYPE_UPLOAD;

	if (!GPUBuffer::Create(a_pDevice, _desc))
	{
		assert(0 && "インデックスバッファ作成時にリソース作成失敗");
		return false;
	}

	Map(&m_pMapData);

	// 仕様書作成
	D3D12_SHADER_RESOURCE_VIEW_DESC _srvDesc = {};
	_srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	_srvDesc.Format = DXGI_FORMAT_UNKNOWN;
	_srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	_srvDesc.Buffer.FirstElement = 0;
	_srvDesc.Buffer.NumElements = m_elementNum;
	_srvDesc.Buffer.StructureByteStride = m_strideSize;
	_srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
	m_srvHandle = AllocateSRV(a_pDevice,GetResource(),_srvDesc);

	return true;
}

void Engine::D3D12::DynamicBuffer::Release()
{
	m_cpResource->Unmap(0, nullptr);
	GPUResource::Release();
}

void Engine::D3D12::DynamicBuffer::UpdateData(const void* a_data, size_t a_size)
{
	std::memcpy(m_pMapData,a_data,a_size);
}

void Engine::D3D12::DynamicBuffer::UpdateDataOffset(const void* a_pData, size_t a_sizeBytes, size_t a_offsetBytes)
{
	// 先頭ポインタからオフセット分ずらしてコピー
	uint8_t* _pDest = static_cast<uint8_t*>(m_pMapData) + a_offsetBytes;
	std::memcpy(_pDest, a_pData, a_sizeBytes);
}