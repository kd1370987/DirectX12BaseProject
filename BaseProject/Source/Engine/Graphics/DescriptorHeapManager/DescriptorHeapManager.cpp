#include "DescriptorHeapManager.h"

#include "../../GPUResource/DescriptorHeap/DSVHeap/DSVHeap.h"
#include "../../GPUResource/DescriptorHeap/RTVHeap/RTVHeap.h"

#include "Engine/Graphics/RenderingEngin/RenderingEngine.h"

void DescriptorHeapManager::Init()
{

	ID3D12Device* _device = RenderingEngine::Instance().GetDevice();

	if(!m_spCBV_SRV_UAVHeap)
	{
		// CBV_SRV_UAV用ディスクリプタヒープの作成
		m_spCBV_SRV_UAVHeap = std::make_shared<CBV_SRV_UAVHeap>();
		m_spCBV_SRV_UAVHeap->Create(
			_device,
			D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
			DirectX::XMFLOAT3(1000.0f, 100.0f, 100.0f),
			D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE
		);
	}

	if (!m_spDSVHeap)
	{
		m_spDSVHeap = std::make_shared<DSVHeap>();
		m_spDSVHeap->Create(
			_device,
			D3D12_DESCRIPTOR_HEAP_TYPE_DSV,
			1,
			D3D12_DESCRIPTOR_HEAP_FLAG_NONE
		);
	}

	if (!m_spRTVHeap)
	{
		m_spRTVHeap = std::make_shared<RTVHeap>();
		m_spRTVHeap->Create(
			_device,
			D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
			BACKBUFFER_COUNT,
			D3D12_DESCRIPTOR_HEAP_FLAG_NONE
		);
	}
}


UINT DescriptorHeapManager::RegisterCBV(
	ID3D12Resource* a_resource, size_t a_size, D3D12_CONSTANT_BUFFER_VIEW_DESC& a_cbvDesc
)
{
	return m_spCBV_SRV_UAVHeap->RegisterCBV(a_resource, a_size, a_cbvDesc);
}

DescriptorHandle DescriptorHeapManager::RegisterSRV(ID3D12Resource* a_resource)
{
	return m_spCBV_SRV_UAVHeap->RegisterSRV(a_resource);
}

DescriptorHandle DescriptorHeapManager::RegisterDSV(ID3D12Resource* a_resource)
{
	
	return m_spDSVHeap->Register(a_resource);
}

DescriptorHandle DescriptorHeapManager::RegisterRTV(ID3D12Resource* a_resource)
{
	
	return m_spRTVHeap->Register(a_resource);
}

