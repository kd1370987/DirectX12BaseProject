#include "GPUBuffer.h"


namespace Engine::D3D12
{
	bool GPUBuffer::Create(D3D12::Device* a_pDevice, const GPUBufferDesc& a_desc)
	{
		// GPUリソースDesc作成
		GPUResourceDesc _desc = {};
		_desc.strideSize = a_desc.strideSize;
		_desc.elementNum = a_desc.elementNum;
		_desc.heapType = a_desc.heapType;

		// バッファ用のリソースDesc作成
		_desc.resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(a_desc.strideSize * a_desc.elementNum, a_desc.flags);

		// バッファなのでクリアバリューはnullptr
		_desc.pClearValue = nullptr;

		// 初期化状態の決定
		_desc.farstState = (a_desc.heapType == D3D12_HEAP_TYPE_UPLOAD)
			? D3D12_RESOURCE_STATE_GENERIC_READ : D3D12_RESOURCE_STATE_COMMON;

		return GPUResource::Create(a_pDevice,_desc);
	}

	void GPUBuffer::Map(void** a_ppData)
	{
		auto _hr = m_cpResource->Map(0, nullptr, a_ppData);

		assert(SUCCEEDED(_hr));
		assert(*a_ppData);
	}

	void GPUBuffer::Unmap()
	{
		GetResource()->Unmap(0,nullptr);
	}

	void GPUBuffer::Write(const void* a_pData, size_t a_size)
	{
		assert(m_cpResource);
		void* _pMappedData = nullptr;
		Map(&_pMappedData);
		std::memcpy(_pMappedData,a_pData,a_size);
		Unmap();
	}
}
