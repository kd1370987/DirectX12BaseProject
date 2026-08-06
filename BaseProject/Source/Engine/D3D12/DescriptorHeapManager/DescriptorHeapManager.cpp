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
		//
		// ヒープ先頭の IMGUI_BACKEND_DESCRIPTOR_COUNT 個はImGuiバックエンド専用に予約し、
		// アプリ側の払い出しはその後ろから始める。
		// 同じ範囲を共有すると、テクスチャを確保し続けたときに
		// フォントアトラスのディスクリプタをモデルのテクスチャで踏み潰してしまう。
		const UINT _imguiHeapSize = a_cbvCount + a_srvCount + a_uavCount;
		m_ImGuiSRVAllocator.Create(
			&m_imguiHeap,
			IMGUI_BACKEND_DESCRIPTOR_COUNT,
			_imguiHeapSize - IMGUI_BACKEND_DESCRIPTOR_COUNT
		);

		// バックエンド予約分の空きインデックスを積む(後ろから取り出して0番から使う)
		m_imguiBackendFreeIndices.clear();
		m_imguiBackendFreeIndices.reserve(IMGUI_BACKEND_DESCRIPTOR_COUNT);
		for (UINT _idx = IMGUI_BACKEND_DESCRIPTOR_COUNT; _idx > 0; --_idx)
		{
			m_imguiBackendFreeIndices.push_back(_idx - 1);
		}

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
		m_ImGuiSRVAllocator.Release();
		m_imguiBackendFreeIndices.clear();

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

	bool DescriptorHeapManager::AllocateImGuiBackendDescriptor(
		D3D12_CPU_DESCRIPTOR_HANDLE* a_pOutCPU,
		D3D12_GPU_DESCRIPTOR_HANDLE* a_pOutGPU
	)
	{
		if (m_imguiBackendFreeIndices.empty())
		{
			ENGINE_ERRLOG(false, "ImGuiバックエンド用ディスクリプタの予約数が足りません");
			return false;
		}

		const UINT _idx = m_imguiBackendFreeIndices.back();
		m_imguiBackendFreeIndices.pop_back();

		if (a_pOutCPU) *a_pOutCPU = m_imguiHeap.GetCPU(_idx);
		if (a_pOutGPU) *a_pOutGPU = m_imguiHeap.GetGPU(_idx);
		return true;
	}

	void DescriptorHeapManager::FreeImGuiBackendDescriptor(D3D12_CPU_DESCRIPTOR_HANDLE a_cpuHandle)
	{
		if (a_cpuHandle.ptr == 0) return;

		// CPUハンドルから予約領域内のインデックスを逆算する
		const D3D12_CPU_DESCRIPTOR_HANDLE _start = m_imguiHeap.GetCPU(0);
		const D3D12_CPU_DESCRIPTOR_HANDLE _next = m_imguiHeap.GetCPU(1);
		const SIZE_T _incrementSize = _next.ptr - _start.ptr;
		if (_incrementSize == 0 || a_cpuHandle.ptr < _start.ptr) return;

		const SIZE_T _offset = a_cpuHandle.ptr - _start.ptr;
		if (_offset % _incrementSize != 0) return;

		const UINT _idx = static_cast<UINT>(_offset / _incrementSize);
		if (_idx >= IMGUI_BACKEND_DESCRIPTOR_COUNT)
		{
			ENGINE_ERRLOG(false, "ImGuiバックエンドの予約範囲外のディスクリプタが返されました");
			return;
		}

		m_imguiBackendFreeIndices.push_back(_idx);
	}

	Handle<ImGuiSRV> DescriptorHeapManager::AllocateImGuiSRV(ID3D12Resource* a_pResource, const D3D12_SHADER_RESOURCE_VIEW_DESC* a_desc)
	{
		auto* _pDevice = D3D12Wrapper::Instance().GetDevice();

		return m_ImGuiSRVAllocator.Allocate(_pDevice, a_pResource, a_desc);
	}

	void DescriptorHeapManager::FreeImGuiSRV(const Handle<ImGuiSRV>& a_handle)
	{
		m_ImGuiSRVAllocator.Remove(a_handle);
	}

	D3D12_CPU_DESCRIPTOR_HANDLE DescriptorHeapManager::GetImGuiSRVCPUHandle(Engine::Handle<ImGuiSRV> a_range)
	{
		return m_ImGuiSRVAllocator.GetCPU(a_range);
	}

	D3D12_GPU_DESCRIPTOR_HANDLE DescriptorHeapManager::GetImGuiSRVGPUHandle(Engine::Handle<ImGuiSRV> a_range)
	{
		return m_ImGuiSRVAllocator.GetGPU(a_range);
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