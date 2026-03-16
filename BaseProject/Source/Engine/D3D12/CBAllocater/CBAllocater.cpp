#include "CBAllocater.h"

#include "Engine/D3D12/DescriptorHeapManager/DescriptorHeapManager.h"

void CBAllocater::RootCBVCreate(ID3D12Device* a_device, size_t a_memSize)
{
	m_pDevice = a_device;
	
	UINT64 _sizeAligned = ((a_memSize + 0xff) & ~0xff);

	auto _prop = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);		// ヒーププロパティ
	auto _desc = CD3DX12_RESOURCE_DESC::Buffer(_sizeAligned);			// リソースの設定

	// リソースを生成
	auto _hr = m_pDevice->CreateCommittedResource(
		&_prop,
		D3D12_HEAP_FLAG_NONE,
		&_desc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(m_spResource.GetAddressOf())
	);
	if (FAILED(_hr))
	{
		assert(0 && "定数バッファリソースの生成に失敗\n");
		return;
	}

	// 定数バッファをマッピング
	_hr = m_spResource->Map(0, nullptr, reinterpret_cast<void**>(&m_pMappedData));

	m_capacity = _sizeAligned;
	m_usedCount = 0;

	CreateCompute(a_memSize);
}

void CBAllocater::CreateCompute(size_t a_memSize)
{
	UINT64 _sizeAligned = ((a_memSize + 0xff) & ~0xff);

	auto _prop = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);		// ヒーププロパティ
	auto _desc = CD3DX12_RESOURCE_DESC::Buffer(_sizeAligned);			// リソースの設定

	// リソースを生成
	auto _hr = m_pDevice->CreateCommittedResource(
		&_prop,
		D3D12_HEAP_FLAG_NONE,
		&_desc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(m_spComputeResource.GetAddressOf())
	);
	if (FAILED(_hr))
	{
		assert(0 && "定数バッファリソースの生成に失敗\n");
		return;
	}

	// 定数バッファをマッピング
	_hr = m_spComputeResource->Map(0, nullptr, reinterpret_cast<void**>(&m_pComputeMappedData));

	m_computeCapacity = _sizeAligned;
	m_useComputeCount = 0;
}
