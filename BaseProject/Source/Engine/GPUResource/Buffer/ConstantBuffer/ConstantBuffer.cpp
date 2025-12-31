#include "ConstantBuffer.h"
#include "Engine/Graphics/DescriptorHeapManager/DescriptorHeapManager.h"

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
	m_handle = DescriptorHeapManager::Instance().RegisterCBV(m_cpBuffer.Get(), _sizeAligned,m_cbvDesc).handleGPU;


	//ID3D12DescriptorHeap* _pHeap = nullptr;
	//D3D12_DESCRIPTOR_HEAP_DESC _heapDesc = {};
	//_heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	//_heapDesc.NumDescriptors = 1;
	//_heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	//_heapDesc.NodeMask = 0;

	//_hr = m_pDevice->CreateDescriptorHeap(&_heapDesc, IID_PPV_ARGS(&_pHeap));
	//if (FAILED(_hr))
	//{
	//	assert(0 && "定数バッファ用ディスクリプタヒープの生成に失敗\n");
	//	return false;
	//}

	//auto _cpuHandle = _pHeap->GetCPUDescriptorHandleForHeapStart();

	//// 定数バッファビューの作成
	//m_cbvDesc.BufferLocation = m_cpBuffer->GetGPUVirtualAddress();
	//m_cbvDesc.SizeInBytes = static_cast<UINT>(_sizeAligned);

	//m_pDevice->CreateConstantBufferView(&m_cbvDesc, _cpuHandle);

	return true;
}

void* ConstantBuffer::GetPtr() const
{
	return m_pMappedPtr;
}

