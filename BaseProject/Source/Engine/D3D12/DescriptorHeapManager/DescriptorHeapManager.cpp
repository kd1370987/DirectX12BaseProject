#include "DescriptorHeapManager.h"

#include "../../D3D12//D3DObject/DescriptorHeap/DSVHeap/DSVHeap.h"
#include "../../D3D12//D3DObject/DescriptorHeap/RTVHeap/RTVHeap.h"

#include "Engine/D3D12/D3D12Wrapper/RenderingEngine.h"

void DescriptorHeapManager::Init()
{

	ID3D12Device* _device = RenderingEngine::Instance().GetDevice();

	if(!m_spCBV_SRV_UAVHeap)
	{
		// CBV_SRV_UAV用ディスクリプタヒープの作成
		m_spCBV_SRV_UAVHeap = std::make_shared<CBV_SRV_UAVHeap>();
		CBV_SRV_UAVInitInfo _info = {};
		_info.pDevice = _device;
		_info.maxCBVCount = 100;
		_info.maxSRVCount = 100;
		_info.maxUAVCount = 100;
		_info.useImGuiSRVCount = 128;
		_info.type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		_info.flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		_info.mask = 0;
		m_spCBV_SRV_UAVHeap->Create(_info);
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

ID3D12DescriptorHeap* DescriptorHeapManager::NGetCBV_SRV_UAVHeap() const
{
	return m_spCBV_SRV_UAVHeap->GetHeap();
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

DescriptorHandle DescriptorHeapManager::AllocateSRVRange(std::vector<ID3D12Resource*> a_resource)
{
	//return m_spCBV_SRV_UAVHeap->AllocateSRVRange(static_cast<UINT>(a_resource.size()));
	return m_spCBV_SRV_UAVHeap->AllocateSRVRange(a_resource);
}

DescriptorHandle DescriptorHeapManager::RegisterDSV(ID3D12Resource* a_resource)
{
	
	return m_spDSVHeap->Register(a_resource);
}

DescriptorHandle DescriptorHeapManager::RegisterRTV(ID3D12Resource* a_resource)
{
	
	return m_spRTVHeap->Register(a_resource);
}

