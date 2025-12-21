#include "ConstantBuffer.h"
#include "Engine/Graphics/RenderingEngin/RenderingEngine.h"
#include "Engine/Graphics/DescriptorHeapManager/DescriptorHeapManager.h"

//ConstantBuffer::ConstantBuffer(size_t a_size)
//{
//	size_t _align = D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT;
//	UINT64 _sizeAligned = (a_size + (_align - 1)) & ~(_align - 1);		// アライメントで切り上げ
//
//	auto _prop = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);		// ヒーププロパティ
//	auto _desc = CD3DX12_RESOURCE_DESC::Buffer(_sizeAligned);			// リソースの設定
//
//	// リソースを生成
//	auto _hr = RenderingEngine::Instance().GetDevice()->CreateCommittedResource(
//		&_prop,
//		D3D12_HEAP_FLAG_NONE,
//		&_desc,
//		D3D12_RESOURCE_STATE_GENERIC_READ,
//		nullptr,
//		IID_PPV_ARGS(m_pBuffer.GetAddressOf())
//	);
//	if (FAILED(_hr))
//	{
//		assert(0 && "定数バッファリソースの生成に失敗\n");
//		return;
//	}
//
//	// 定数バッファをマッピング
//	_hr = m_pBuffer->Map(0, nullptr, &m_pMappedPtr);
//	if (FAILED(_hr))
//	{
//		assert(0 && "定数バッファのマッピングに失敗\n");
//		return;
//	}
//
//	m_desc = {};
//	m_desc.BufferLocation = m_pBuffer->GetGPUVirtualAddress();
//	m_desc.SizeInBytes = UINT(_sizeAligned);
//
//	m_handle = DescriptorHeapManager::Instance().RegisterCBV(m_pBuffer.Get(), _sizeAligned).handleGPU;
//
//	m_isValid = true;
//}

ConstantBuffer::ConstantBuffer()
{
}

void ConstantBuffer::Create(size_t a_size)
{
	assert(!m_isValid && "ConstantBuffer::Create 二重呼び");

	size_t _align = D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT;
	UINT64 _sizeAligned = (a_size + (_align - 1)) & ~(_align - 1);		// アライメントで切り上げ

	auto _prop = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);		// ヒーププロパティ
	auto _desc = CD3DX12_RESOURCE_DESC::Buffer(_sizeAligned);			// リソースの設定

	// リソースを生成
	auto _hr = RenderingEngine::Instance().GetDevice()->CreateCommittedResource(
		&_prop,
		D3D12_HEAP_FLAG_NONE,
		&_desc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(m_pBuffer.GetAddressOf())
	);
	if (FAILED(_hr))
	{
		assert(0 && "定数バッファリソースの生成に失敗\n");
		return;
	}

	// 定数バッファをマッピング
	_hr = m_pBuffer->Map(0, nullptr, &m_pMappedPtr);
	if (FAILED(_hr))
	{
		assert(0 && "定数バッファのマッピングに失敗\n");
		return;
	}

	m_desc = {};
	m_desc.BufferLocation = m_pBuffer->GetGPUVirtualAddress();
	m_desc.SizeInBytes = UINT(_sizeAligned);

	m_handle = DescriptorHeapManager::Instance().RegisterCBV(m_pBuffer.Get(), _sizeAligned).handleGPU;

	m_isValid = true;
}

bool ConstantBuffer::IsValid()
{
	return m_isValid;
}

D3D12_GPU_VIRTUAL_ADDRESS ConstantBuffer::GetAddres() const
{
	return m_desc.BufferLocation;
}

D3D12_CONSTANT_BUFFER_VIEW_DESC ConstantBuffer::ViewDesc()
{
	return m_desc;
}

void* ConstantBuffer::GetPtr() const
{
	return m_pMappedPtr;
}

void ConstantBuffer::UnMap()
{
	m_pBuffer->Unmap(0, nullptr);
}
