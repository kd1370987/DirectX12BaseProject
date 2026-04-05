#include "DescriptorHeapManager.h"
#include "Engine/D3D12/D3D12Wrapper/D3D12Wrapper.h"

// ヒープアロケーター
#include "Engine/D3D12/DescriptorHeapManager/Allocater/RTVAllocator/RTVAllocator.h"
#include "Engine/D3D12/DescriptorHeapManager/Allocater/DSVAllocator/DSVAllocator.h"
#include "Engine/D3D12/DescriptorHeapManager/Allocater/SRVAllocator/SRVAllocator.h"
#include "Engine/D3D12/DescriptorHeapManager/Allocater/UAVAllocator/UAVAllocator.h"
#include "Engine/D3D12/DescriptorHeapManager/Allocater/SamplerAllocator/SamplerAllocator.h"

bool DescriptorHeapManager::Init()
{
	ID3D12Device* _device = D3D12Wrapper::Instance().GetDevice();

	// ヒープ作成
	m_cbv_srv_uavHeap.Create(
		_device,
		L"CBV_SRV_UAV",
		300,
		D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
		0
	);
	m_dsvHeap.Create(
		_device,
		L"DSV",
		10,
		D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
		0
	);
	m_rtvHeap.Create(
		_device,
		L"RTV",
		100,
		D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
		0
	);
	m_imguiHeap.Create(
		_device,
		L"ImGui",
		300,
		D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
		0
	);
	m_samplerHeap.Create(
		_device,
		L"Sampler",
		3,
		D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
		0
	);

	// アロケーター生成
	// RTV
	m_upRTVAllocator = std::make_unique<Engine::D3D12::RTVAllocator>();
	m_upRTVAllocator->Create(&m_rtvHeap);

	// DSV
	m_upDSVAllocator = std::make_unique<Engine::D3D12::DSVAllocator>();
	m_upDSVAllocator->Create(&m_dsvHeap);

	// SRV
	m_upSRVAllocator = std::make_unique<Engine::D3D12::SRVAllocator>();
	m_upSRVAllocator->Create(&m_cbv_srv_uavHeap,100,100);		// 100番目から100個の容量

	// UAV
	m_upUAVAllocator = std::make_unique<Engine::D3D12::UAVAllocator>();
	m_upUAVAllocator->Create(&m_cbv_srv_uavHeap,200,100);		// 200番目から100個の容量

	// ImGUI用SRV
	m_upImGuiSRVAllocator = std::make_unique<Engine::D3D12::SRVAllocator>();
	m_upImGuiSRVAllocator->Create(&m_imguiHeap,0,300);

	// Sampler
	m_upSamplerAllocator = std::make_unique<Engine::D3D12::SamplerAllocator>();
	m_upSamplerAllocator->Create(&m_samplerHeap);

	// 主要サンプラー作成
	D3D12_SAMPLER_DESC _linerDesc = {};
	_linerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	_linerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	_linerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	_linerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	m_linerWrap = CreateSampler(_device,_linerDesc);
	D3D12_SAMPLER_DESC _pointDesc = {};
	_pointDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
	_pointDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	_pointDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	_pointDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	m_pointClamp = CreateSampler(_device,_pointDesc);
	D3D12_SAMPLER_DESC _shadowDesc = {};
	_shadowDesc.Filter = D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
	_shadowDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	_shadowDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	_shadowDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	_shadowDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
	m_shadow = CreateSampler(_device,_shadowDesc);


	return true;
}

void DescriptorHeapManager::Release()
{
}

std::vector<Engine::Resource::Handle<SRV>> DescriptorHeapManager::AllocateSRVRange(std::vector<SRVViewInit> a_viewInitVec)
{
	return m_upSRVAllocator->Allocate(a_viewInitVec);
}

D3D12_CPU_DESCRIPTOR_HANDLE DescriptorHeapManager::GetSRVCPUHandle(Engine::Resource::Handle<SRV> a_range)
{
	return m_upSRVAllocator->GetCPU(a_range);
}

D3D12_GPU_DESCRIPTOR_HANDLE DescriptorHeapManager::GetSRVGPUHandle(Engine::Resource::Handle<SRV> a_range)
{
	return m_upSRVAllocator->GetGPU(a_range);
}

std::vector<Engine::Resource::Handle<UAV>> DescriptorHeapManager::AllocateUAVRange(std::vector<UAVViewInit> a_viewInitVec)
{
	return m_upUAVAllocator->Allocate(a_viewInitVec);
}

D3D12_CPU_DESCRIPTOR_HANDLE DescriptorHeapManager::GetUAVCPUHandle(Engine::Resource::Handle<UAV> a_range)
{
	return m_upUAVAllocator->GetCPU(a_range);
}

D3D12_GPU_DESCRIPTOR_HANDLE DescriptorHeapManager::GetUAVGPUHandle(Engine::Resource::Handle<UAV> a_range)
{
	return m_upUAVAllocator->GetGPU(a_range);
}

ID3D12DescriptorHeap* DescriptorHeapManager::GetCBV_SRV_UAVHeap() const
{
	return m_cbv_srv_uavHeap.GetHeap();
}

ID3D12DescriptorHeap* DescriptorHeapManager::GetImGuiHeap() const
{
	return m_imguiHeap.GetHeap();
}

std::vector<Engine::Resource::Handle<SRV>> DescriptorHeapManager::AllocateImGuiSRVRange(std::vector<SRVViewInit> a_viewInitVec)
{
	return m_upImGuiSRVAllocator->Allocate(a_viewInitVec);
}

D3D12_CPU_DESCRIPTOR_HANDLE DescriptorHeapManager::GetImGuiCPUHandle()
{
	return m_imguiHeap.GetCPU(100);
}

D3D12_GPU_DESCRIPTOR_HANDLE DescriptorHeapManager::GetImGuiGPUHandle()
{
	return m_imguiHeap.GetGPU(100);
}

D3D12_CPU_DESCRIPTOR_HANDLE DescriptorHeapManager::GetImGuiSRVCPUHandle(Engine::Resource::Handle<SRV> a_range)
{
	return m_upImGuiSRVAllocator->GetCPU(a_range);
}

D3D12_GPU_DESCRIPTOR_HANDLE DescriptorHeapManager::GetImGuiSRVGPUHandle(Engine::Resource::Handle<SRV> a_range)
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

Engine::Resource::Handle<SAMPLER> DescriptorHeapManager::CreateSampler(ID3D12Device* a_pDevice, const D3D12_SAMPLER_DESC& a_desc)
{
	return m_upSamplerAllocator->Allocate(a_pDevice,a_desc);
}

D3D12_GPU_DESCRIPTOR_HANDLE DescriptorHeapManager::GetLinearWrap()
{
	return m_upSamplerAllocator->GetGPU(m_linerWrap);;
}

D3D12_GPU_DESCRIPTOR_HANDLE DescriptorHeapManager::GetPointClamp()
{
	return m_upSamplerAllocator->GetGPU(m_pointClamp);;
}

D3D12_GPU_DESCRIPTOR_HANDLE DescriptorHeapManager::GetShadow()
{
	return m_upSamplerAllocator->GetGPU(m_shadow);;
}

ID3D12DescriptorHeap* DescriptorHeapManager::RefSamplerHeap()
{
	return m_samplerHeap.GetHeap();
}

DescriptorHeapManager::DescriptorHeapManager()
{
}

DescriptorHeapManager::~DescriptorHeapManager()
{
}




