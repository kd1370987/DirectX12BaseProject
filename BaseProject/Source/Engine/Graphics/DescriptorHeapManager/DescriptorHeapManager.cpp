#include "DescriptorHeapManager.h"

#include "Engine/GPUResource/DescriptorHeap/DescriptorHeap.h"

void DescriptorHeapManager::Init()
{
	// SRV用ディスクリプタヒープの作成
	m_spSRVHeap = std::make_shared<DescriptorHeap>();
	m_spSRVHeap->Create(
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
		512,
		D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE
	);
}

DescriptorHandle DescriptorHeapManager::RegisterSRV(ID3D12Resource* a_resource)
{
	return m_spSRVHeap->RegisterSRV(a_resource);	
}
