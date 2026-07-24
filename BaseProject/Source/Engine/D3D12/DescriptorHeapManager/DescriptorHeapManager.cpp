#include "DescriptorHeapManager.h"
#include "Engine/D3D12/D3D12Wrapper/D3D12Wrapper.h"

#include "Allocator/SamplerAllocator/SamplerAllocator.h"

namespace Engine::D3D12
{
	bool DescriptorHeapManager::Init(UINT a_cbvCount, UINT a_srvCount, UINT a_uavCount, UINT a_rtvCount, UINT a_dsvCount)
	{
		D3D12::Device* _device = D3D12Wrapper::Instance().GetDevice();

		// ヒープ作成
		m_cbv_srv_uavHeap.Create(
			_device,
			L"CBV_SRV_UAV",
			a_cbvCount + a_srvCount + a_uavCount,
			D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
			0
		);
		m_dsvHeap.Create(
			_device,
			L"DSV",
			a_dsvCount,
			D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
			0
		);
		m_rtvHeap.Create(
			_device,
			L"RTV",
			a_rtvCount,
			D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
			0
		);
		m_imguiHeap.Create(
			_device,
			L"ImGui",
			a_cbvCount + a_srvCount + a_uavCount,
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
		UINT _startIdx = 0;
		m_CBVAllocator.Create(&m_cbv_srv_uavHeap, _startIdx, a_cbvCount);	// CBV
		_startIdx += a_cbvCount;
		m_SRVAllocator.Create(&m_cbv_srv_uavHeap, _startIdx, a_srvCount);	// SRV
		_startIdx += a_srvCount;
		m_UAVAllocator.Create(&m_cbv_srv_uavHeap, _startIdx, a_uavCount);	// UAV

		m_RTVAllocator.Create(&m_rtvHeap);	// RTV
		m_DSVAllocator.Create(&m_dsvHeap);	// DSV

		// ImGUI用SRV
		m_ImGuiSRVAllocator.Create(&m_imguiHeap, 0, a_cbvCount + a_srvCount + a_uavCount);

		// Sampler
		m_upSamplerAllocator = std::make_unique<Engine::D3D12::SamplerAllocator>();
		m_upSamplerAllocator->Create(&m_samplerHeap);

		// 主要サンプラー作成
		D3D12_SAMPLER_DESC _linerDesc = {};
		_linerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
		_linerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		_linerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		_linerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		m_linerWrap = CreateSampler(_device, _linerDesc);
		D3D12_SAMPLER_DESC _pointDesc = {};
		_pointDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
		_pointDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		_pointDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		_pointDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		m_pointClamp = CreateSampler(_device, _pointDesc);
		D3D12_SAMPLER_DESC _shadowDesc = {};
		_shadowDesc.Filter = D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
		_shadowDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		_shadowDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		_shadowDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		_shadowDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
		m_shadow = CreateSampler(_device, _shadowDesc);



		return true;
	}

	void DescriptorHeapManager::Release()
	{
		// アロケーターのリンク解除
		m_CBVAllocator.Release();
		m_SRVAllocator.Release();
		m_UAVAllocator.Release();
		m_RTVAllocator.Release();
		m_DSVAllocator.Release();

		m_upSamplerAllocator->Release();
		m_upSamplerAllocator.reset();

		// ヒープの解放
		m_cbv_srv_uavHeap.Release();
		m_dsvHeap.Release();
		m_rtvHeap.Release();
		m_samplerHeap.Release();
		m_imguiHeap.Release();
	}

	
	UINT DescriptorHeapManager::GetCBVSRVUAVHeapSize()
	{
		return m_cbv_srv_uavHeap.GetMaxSize();
	}

	ID3D12DescriptorHeap* DescriptorHeapManager::GetCBVSRVUAVHeap()
	{
		return m_cbv_srv_uavHeap.GetHeap();
	}

	ID3D12DescriptorHeap* DescriptorHeapManager::GetImGuiHeap() const
	{
		return m_imguiHeap.GetHeap();
	}

	D3D12_CPU_DESCRIPTOR_HANDLE DescriptorHeapManager::GetImGuiCPUHandle()
	{
		return m_imguiHeap.GetCPU(100);
	}

	D3D12_GPU_DESCRIPTOR_HANDLE DescriptorHeapManager::GetImGuiGPUHandle()
	{
		return m_imguiHeap.GetGPU(100);
	}

	Handle<SRV> DescriptorHeapManager::AllocateImGuiSRV(ID3D12Resource* a_pResource, const D3D12_SHADER_RESOURCE_VIEW_DESC* a_desc)
	{
		auto* _pDevice = D3D12Wrapper::Instance().GetDevice();

		return m_ImGuiSRVAllocator.Allocate(_pDevice, a_pResource, a_desc);
	}

	void DescriptorHeapManager::FreeImGuiSRV(const Handle<SRV>& a_handle)
	{
		m_ImGuiSRVAllocator.Remove(a_handle);
	}

	D3D12_CPU_DESCRIPTOR_HANDLE DescriptorHeapManager::GetImGuiSRVCPUHandle(Engine::Handle<SRV> a_range)
	{
		return m_ImGuiSRVAllocator.GetCPU(a_range);
	}

	D3D12_GPU_DESCRIPTOR_HANDLE DescriptorHeapManager::GetImGuiSRVGPUHandle(Engine::Handle<SRV> a_range)
	{
		return m_ImGuiSRVAllocator.GetGPU(a_range);;
	}

	Engine::Handle<SAMPLER> DescriptorHeapManager::CreateSampler(D3D12::Device* a_pDevice, const D3D12_SAMPLER_DESC& a_desc)
	{
		return m_upSamplerAllocator->Allocate(a_pDevice, a_desc);
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
	{}

	DescriptorHeapManager::~DescriptorHeapManager()
	{}




}