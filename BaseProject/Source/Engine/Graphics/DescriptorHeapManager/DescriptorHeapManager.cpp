#include "DescriptorHeapManager.h"

#include "../../GPUResource/DescriptorHeap/CBVHeap/CBVHeap.h"
#include "../../GPUResource/DescriptorHeap/SRVHeap/SRVHeap.h"
#include "../../GPUResource/DescriptorHeap/DSVHeap/DSVHeap.h"
#include "../../GPUResource/DescriptorHeap/RTVHeap/RTVHeap.h"

DescriptorHandle DescriptorHeapManager::RegisterCBV(ID3D12Resource* a_resource)
{
	// ディスクリプタヒープがまだ作成されていなかったら作成
	if (!m_spCBVHeap)
	{
		// CBV用ディスクリプタヒープの作成
		m_spCBVHeap = std::make_shared<CBVHeap>();
		m_spCBVHeap->Create(
			D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
			100,
			D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE
		);
	}
	return m_spCBVHeap->Register(a_resource);
}

DescriptorHandle DescriptorHeapManager::RegisterSRV(ID3D12Resource* a_resource)
{
	// ディスクリプタヒープがまだ作成されていなかったら作成
	if (!m_spSRVHeap)
	{
		// SRV用ディスクリプタヒープの作成
		m_spSRVHeap = std::make_shared<SRVHeap>();
		m_spSRVHeap->Create(
			D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
			100,
			D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE
		);
	}
	return m_spSRVHeap->Register(a_resource);	
}

DescriptorHandle DescriptorHeapManager::RegisterDSV(ID3D12Resource* a_resource)
{
	if (!m_spDSVHeap)
	{
		m_spDSVHeap = std::make_shared<DSVHeap>();
		m_spDSVHeap->Create(
			D3D12_DESCRIPTOR_HEAP_TYPE_DSV,
			1,
			D3D12_DESCRIPTOR_HEAP_FLAG_NONE
		);
	}
	return m_spDSVHeap->Register(a_resource);
}

DescriptorHandle DescriptorHeapManager::RegisterRTV(ID3D12Resource* a_resource)
{
	if (!m_spRTVHeap)
	{
		m_spRTVHeap = std::make_shared<RTVHeap>();
		m_spRTVHeap->Create(
			D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
			FRAME_BUFFER_COUNT,
			D3D12_DESCRIPTOR_HEAP_FLAG_NONE
		);
	}
	return m_spRTVHeap->Register(a_resource);
}

