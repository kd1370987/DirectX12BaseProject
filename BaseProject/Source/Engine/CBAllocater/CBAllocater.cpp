#include "CBAllocater.h"

#include "Engine/Graphics/DescriptorHeapManager/DescriptorHeapManager.h"

void CBAllocater::Init(ID3D12Device* a_device)
{
	m_pDevice = a_device;
	m_pHeap = DescriptorHeapManager::Instance().GetDescriptorCBV_SRV_UAV().get();

	//UINT64 _sizeAligned = ((1 + 0xff) & ~0xff) * m_pHeap->GetMaxCounts().x;
	UINT64 _sizeAligned = ((1024 + 0xff) & ~0xff) * m_pHeap->GetMaxCounts().x;

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
}
