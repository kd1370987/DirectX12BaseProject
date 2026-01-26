#include "ConstantBuffer.h"
#include "Engine/D3D12/DescriptorHeapManager/DescriptorHeapManager.h"

ConstantBuffer::ConstantBuffer(ID3D12Device* a_pDevice) : m_pDevice(a_pDevice)
{
}

bool ConstantBuffer::Create(size_t a_size)
{
	assert(!m_cpBuffer && "ConstantBuffer::Create 二重呼び");

	size_t _align = D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT;
	UINT64 _sizeAligned = (a_size + (_align - 1)) & ~(_align - 1);		// アライメントで切り上げ

	auto _prop = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);		// ヒーププロパティ
	auto _desc = CD3DX12_RESOURCE_DESC::Buffer(_sizeAligned);			// リソースの設定

	// リソースを生成
	auto _hr = m_pDevice->CreateCommittedResource(
		&_prop,
		D3D12_HEAP_FLAG_NONE,
		&_desc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(m_cpBuffer.GetAddressOf())
	);
	if (FAILED(_hr))
	{
		assert(0 && "定数バッファリソースの生成に失敗\n");
		return false;
	}

	// 定数バッファをマッピング
	_hr = m_cpBuffer->Map(0, nullptr, &m_pMappedPtr);
	if (FAILED(_hr))
	{
		assert(0 && "定数バッファのマッピングに失敗\n");
		return false;
	}

	m_cbvDesc = {};
	
	return true;
}

void* ConstantBuffer::GetPtr() const
{
	return m_pMappedPtr;
}

