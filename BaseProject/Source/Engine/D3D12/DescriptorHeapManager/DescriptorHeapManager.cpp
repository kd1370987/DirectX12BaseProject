#include "DescriptorHeapManager.h"
#include "Engine/D3D12/D3D12Wrapper/D3D12Wrapper.h"

// ヒープアロケーター
#include "Engine/D3D12/DescriptorHeapManager/Allocater/RTVAllocator/RTVAllocator.h"
#include "Engine/D3D12/DescriptorHeapManager/Allocater/DSVAllocator/DSVAllocator.h"
#include "Engine/D3D12/DescriptorHeapManager/Allocater/SRVAllocator/SRVAllocator.h"

bool DescriptorHeapManager::Init()
{
	ID3D12Device* _device = D3D12Wrapper::Instance().GetDevice();

	// ヒープ作成
	m_cbv_srv_uavHeap.Create(
		_device,
		300,
		D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
		0
	);
	m_dsvHeap.Create(
		_device,
		10,
		D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
		0
	);
	m_rtvHeap.Create(
		_device,
		100,
		D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
		0
	);
	m_imguiHeap.Create(
		_device,
		300,
		D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
		0
	);

	// アロケーター生成
	m_upRTVAllocator = std::make_unique<Engine::D3D12::RTVAllocator>();
	m_upRTVAllocator->Create(&m_rtvHeap);

	m_upDSVAllocator = std::make_unique<Engine::D3D12::DSVAllocator>();
	m_upDSVAllocator->Create(&m_dsvHeap);

	m_upSRVAllocator = std::make_unique<Engine::D3D12::SRVAllocator>();
	m_upSRVAllocator->Create(&m_cbv_srv_uavHeap,100,100);

	m_upImGuiSRVAllocator = std::make_unique<Engine::D3D12::SRVAllocator>();
	m_upImGuiSRVAllocator->Create(&m_imguiHeap,0,300);

	return true;
}

void DescriptorHeapManager::Release()
{
}

Engine::Resource::HandleRange<SRV> DescriptorHeapManager::AllocateSRVRange(std::vector<SRVViewInit> a_viewInitVec)
{	
	return m_upSRVAllocator->Allocate(a_viewInitVec);
}

D3D12_CPU_DESCRIPTOR_HANDLE DescriptorHeapManager::GetSRVCPUHandle(Engine::Resource::HandleRange<SRV> a_range)
{
	return m_upSRVAllocator->GetCPU(a_range);
}

D3D12_GPU_DESCRIPTOR_HANDLE DescriptorHeapManager::GetSRVGPUHandle(Engine::Resource::HandleRange<SRV> a_range)
{
	return m_upSRVAllocator->GetGPU(a_range);
}

ID3D12DescriptorHeap* DescriptorHeapManager::GetCBV_SRV_UAVHeap() const
{
	return m_cbv_srv_uavHeap.GetHeap();
}

ID3D12DescriptorHeap* DescriptorHeapManager::GetImGuiHeap() const
{
	return m_imguiHeap.GetHeap();
}

Engine::Resource::HandleRange<SRV> DescriptorHeapManager::AllocateImGuiSRVRange(std::vector<SRVViewInit> a_viewInitVec)
{
	return m_upImGuiSRVAllocator->Allocate(a_viewInitVec);
}

D3D12_CPU_DESCRIPTOR_HANDLE DescriptorHeapManager::GetImGuiCPUHandle()
{
	return m_imguiHeap.GetCPU(0);
}

D3D12_GPU_DESCRIPTOR_HANDLE DescriptorHeapManager::GetImGuiGPUHandle()
{
	return m_imguiHeap.GetGPU(0);
}

D3D12_CPU_DESCRIPTOR_HANDLE DescriptorHeapManager::GetImGuiSRVCPUHandle(Engine::Resource::HandleRange<SRV> a_range)
{
	return m_upImGuiSRVAllocator->GetCPU(a_range);
}

D3D12_GPU_DESCRIPTOR_HANDLE DescriptorHeapManager::GetImGuiSRVGPUHandle(Engine::Resource::HandleRange<SRV> a_range)
{
	return m_upImGuiSRVAllocator->GetGPU(a_range);;
}

Engine::Resource::Handle<DSV> DescriptorHeapManager::AllocateDSV(
	ID3D12Resource* a_resource, 
	D3D12_DEPTH_STENCIL_VIEW_DESC* a_pDSVDesc
)
{
	return m_upDSVAllocator->Allocate(a_resource,a_pDSVDesc);
}

void DescriptorHeapManager::RemoveDSV(Engine::Resource::Handle<DSV> a_handle)
{
	m_upDSVAllocator->Remove(a_handle);
}

D3D12_CPU_DESCRIPTOR_HANDLE DescriptorHeapManager::GetDSVCPUHandle(
	Engine::Resource::Handle<DSV> a_handle
)
{
	return m_upDSVAllocator->GetCPU(a_handle);
}

Engine::Resource::Handle<RTV> DescriptorHeapManager::AllocateRTV(
	ID3D12Resource* a_resource, D3D12_RENDER_TARGET_VIEW_DESC* a_pRtvDesc
)
{
	return m_upRTVAllocator->Allocate(a_resource,a_pRtvDesc);
}

void DescriptorHeapManager::RemoveRTV(Engine::Resource::Handle<RTV> a_handle)
{
	m_upRTVAllocator->Remove(a_handle);
}

D3D12_CPU_DESCRIPTOR_HANDLE DescriptorHeapManager::GetRTVCPUHandle(Engine::Resource::Handle<RTV> a_handle)
{
	return m_upRTVAllocator->GetCPU(a_handle);
}

DescriptorHeapManager::DescriptorHeapManager()
{
}

DescriptorHeapManager::~DescriptorHeapManager()
{
}




