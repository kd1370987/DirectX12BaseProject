#include "DescriptorHeapManager.h"

#include "../../D3D12//D3DObject/DescriptorHeap/DSVHeap/DSVHeap.h"
#include "../../D3D12//D3DObject/DescriptorHeap/RTVHeap/RTVHeap.h"
#include "../../D3D12//D3DObject/DescriptorHeap/CBV_SRV_UAVHeap/CBV_SRV_UAVHeap.h"

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
			100,
			D3D12_DESCRIPTOR_HEAP_FLAG_NONE
		);
	}
}


Storage::Range DescriptorHeapManager::RegisterSRV(ID3D12Resource* a_resource)
{
	return m_spCBV_SRV_UAVHeap->AllocateSRVRange({ {a_resource,nullptr} });
}

Storage::Range DescriptorHeapManager::AllocateSRVRange(std::vector<SRVViewInit> a_viewInitVec)
{
	return m_spCBV_SRV_UAVHeap->AllocateSRVRange(a_viewInitVec);
}

D3D12_CPU_DESCRIPTOR_HANDLE DescriptorHeapManager::GetSRVCPUHandle(Storage::Range a_range)
{
	return m_spCBV_SRV_UAVHeap->GetSRVCPUHandle(a_range);
}

D3D12_GPU_DESCRIPTOR_HANDLE DescriptorHeapManager::GetSRVGPUHandle(Storage::Range a_range)
{
	return m_spCBV_SRV_UAVHeap->GetSRVGPUHandle(a_range);
}

ID3D12DescriptorHeap* DescriptorHeapManager::GetCBV_SRV_UAVHeap() const
{
	return m_spCBV_SRV_UAVHeap->GetHeap();
}

D3D12_CPU_DESCRIPTOR_HANDLE DescriptorHeapManager::GetImGuiCPUHandle()
{
	return m_spCBV_SRV_UAVHeap->GetImGuiCPUHandle();
}

D3D12_GPU_DESCRIPTOR_HANDLE DescriptorHeapManager::GetImGuiGPUHandle()
{
	return m_spCBV_SRV_UAVHeap->GetImGuiGPUHandle();
}

DescriptorHandle DescriptorHeapManager::RegisterDSV(ID3D12Resource* a_resource)
{
	
	return m_spDSVHeap->Register(a_resource);
}

D3D12_CPU_DESCRIPTOR_HANDLE DescriptorHeapManager::GetCPUDSV()
{
	return m_spDSVHeap->GetHeap()->GetCPUDescriptorHandleForHeapStart();
}

RTVHandle DescriptorHeapManager::RegisterRTV(ID3D12Resource* a_resource, D3D12_RENDER_TARGET_VIEW_DESC* a_pRtvDesc)
{
	return m_spRTVHeap->RegisterRTV(a_resource,a_pRtvDesc);
}

D3D12_CPU_DESCRIPTOR_HANDLE DescriptorHeapManager::GetRTVCPUHandle(RTVHandle a_handle)
{
	return m_spRTVHeap->GetCPUHandle(a_handle);
}


