#pragma once

#include "Allocater/HeapAllocater.h"

namespace Engine::D3D12
{
	// 前方宣言
	class SamplerAllocator;

	// ディスクリプタヒープを管理
	class DescriptorHeapManager
	{
	public:

		// 初期化と解放
		bool Init(
			UINT a_cbvCount,
			UINT a_srvCount,
			UINT a_uavCount,
			UINT a_rtvCount,
			UINT a_dsvCount
		);
		void Release();

		// リソースのビュー作成
		template<IsHeapType T>
		Handle<T> Allocate(D3D12::Device* a_pDevice,ID3D12Resource* a_pResource,const typename T::DescType* a_desc);

		// ビューの解放
		template<IsHeapType T>
		void Free(const Handle<T>& a_handle);

		// ハンドルの取得
		template<IsHeapType T>
		D3D12_CPU_DESCRIPTOR_HANDLE GetCPU(Handle<T> a_handle);
		template<IsHeapType T>
		D3D12_GPU_DESCRIPTOR_HANDLE GetGPU(Handle<T> a_handle);

		// ヒープ取得
		UINT GetCBVSRVUAVHeapSize();
		ID3D12DescriptorHeap* GetCBVSRVUAVHeap();

		//==========================================================================================
		// 
		// ImGui
		// 
		//==========================================================================================

		// ImGui初期設定用
		ID3D12DescriptorHeap* GetImGuiHeap() const;
		D3D12_CPU_DESCRIPTOR_HANDLE GetImGuiCPUHandle();
		D3D12_GPU_DESCRIPTOR_HANDLE GetImGuiGPUHandle();

		// 一括でSRVを確保
		Handle<SRV> AllocateImGuiSRV(ID3D12Resource* a_pResource, const D3D12_SHADER_RESOURCE_VIEW_DESC* a_desc);


		// 解放
		void FreeImGuiSRV(const Handle<SRV>& a_handle);

		// ImGuiのSRVハンドルを取得
		D3D12_CPU_DESCRIPTOR_HANDLE GetImGuiSRVCPUHandle(Engine::Handle<SRV> a_range);
		D3D12_GPU_DESCRIPTOR_HANDLE GetImGuiSRVGPUHandle(Engine::Handle<SRV> a_range);

		//==========================================================================================
		// 
		// SAMPLER
		// 
		//==========================================================================================
		// 作成
		Engine::Handle<SAMPLER> CreateSampler(
			D3D12::Device* a_pDevice,
			const D3D12_SAMPLER_DESC& a_desc
		);

		// 取得
		D3D12_GPU_DESCRIPTOR_HANDLE GetLinearWrap();
		D3D12_GPU_DESCRIPTOR_HANDLE GetPointClamp();
		D3D12_GPU_DESCRIPTOR_HANDLE GetShadow();

		// ヒープ
		ID3D12DescriptorHeap* RefSamplerHeap();


	private:

		// ヒープ本体
		Engine::D3D12::DescriptorHeap<D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV>	m_cbv_srv_uavHeap;
		Engine::D3D12::DescriptorHeap<D3D12_DESCRIPTOR_HEAP_TYPE_DSV>			m_dsvHeap;
		Engine::D3D12::DescriptorHeap<D3D12_DESCRIPTOR_HEAP_TYPE_RTV>			m_rtvHeap;

		Engine::D3D12::DescriptorHeap<D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER>		m_samplerHeap;
		Engine::D3D12::DescriptorHeap<D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV>	m_imguiHeap;

		// ヒープアロケーター
		HeapAllocator<CBV>		m_CBVAllocator;
		HeapAllocator<SRV>		m_SRVAllocator;
		HeapAllocator<UAV>		m_UAVAllocator;

		HeapAllocator<RTV>		m_RTVAllocator;
		HeapAllocator<DSV>		m_DSVAllocator;
		
		std::unique_ptr<SamplerAllocator>	m_upSamplerAllocator = nullptr;
		HeapAllocator<SRV>					m_ImGuiSRVAllocator;

		// サンプラー
		Engine::Handle<SAMPLER> m_linerWrap;
		Engine::Handle<SAMPLER> m_pointClamp;
		Engine::Handle<SAMPLER> m_shadow;

		// シングルトン
	private:
		DescriptorHeapManager();
		~DescriptorHeapManager();

		// コピー禁止
		DescriptorHeapManager(const DescriptorHeapManager&) = delete;
		void operator=(const DescriptorHeapManager&) = delete;

	public:

		static DescriptorHeapManager& Instance()
		{
			static DescriptorHeapManager instance;
			return instance;
		}
	};
	template<IsHeapType T>
	inline Handle<T> DescriptorHeapManager::Allocate(D3D12::Device* a_pDevice, ID3D12Resource* a_pResource, const typename T::DescType* a_desc)
	{
		if constexpr (std::is_same_v<T, CBV>)
		{
			return m_CBVAllocator.Allocate(a_pDevice, a_pResource, a_desc);
		}
		else if constexpr (std::is_same_v<T, SRV>)
		{
			return m_SRVAllocator.Allocate(a_pDevice, a_pResource, a_desc);
		}
		else if constexpr (std::is_same_v<T, UAV>)
		{
			return m_UAVAllocator.Allocate(a_pDevice, a_pResource, a_desc);
		}
		else if constexpr (std::is_same_v<T, RTV>)
		{
			return m_RTVAllocator.Allocate(a_pDevice, a_pResource, a_desc);
		}
		else if constexpr (std::is_same_v<T, DSV>)
		{
			return m_DSVAllocator.Allocate(a_pDevice, a_pResource, a_desc);
		}
		//else
		//{
		//	static_assert(slways_false_v<T>,"Unsupported Heap Type");
		//	return {};
		//}
	}
	template<IsHeapType T>
	inline void DescriptorHeapManager::Free(const Handle<T>& a_handle)
	{
		if constexpr (std::is_same_v<T, CBV>)
		{
			ENGINE_LOG("CBVの解放 : Index %d,Generation %d", static_cast<int>(a_handle.GetIndex()), static_cast<int>(a_handle.GetGeneration()));
			return m_CBVAllocator.Remove(a_handle);
		}
		else if constexpr (std::is_same_v<T, SRV>)
		{
			ENGINE_LOG("SRVの解放 : Index %d,Generation %d", static_cast<int>(a_handle.GetIndex()), static_cast<int>(a_handle.GetGeneration()));
			return m_SRVAllocator.Remove(a_handle);
		}
		else if constexpr (std::is_same_v<T, UAV>)
		{
			ENGINE_LOG("UAVの解放 : Index %d,Generation %d", static_cast<int>(a_handle.GetIndex()), static_cast<int>(a_handle.GetGeneration()));
			return m_UAVAllocator.Remove(a_handle);
		}
		else if constexpr (std::is_same_v<T, RTV>)
		{
			ENGINE_LOG("RTVの解放 : Index %d,Generation %d", static_cast<int>(a_handle.GetIndex()), static_cast<int>(a_handle.GetGeneration()));
			return m_RTVAllocator.Remove(a_handle);
		}
		else if constexpr (std::is_same_v<T, DSV>)
		{
			ENGINE_LOG("DSVの解放 : Index %d,Generation %d", static_cast<int>(a_handle.GetIndex()), static_cast<int>(a_handle.GetGeneration()));
			return m_DSVAllocator.Remove(a_handle);
		}
	}
	template<IsHeapType T>
	inline D3D12_CPU_DESCRIPTOR_HANDLE DescriptorHeapManager::GetCPU(Handle<T> a_handle)
	{
		if constexpr (std::is_same_v<T, CBV>)
		{
			return m_CBVAllocator.GetCPU(a_handle);
		}
		else if constexpr (std::is_same_v<T, SRV>)
		{
			return m_SRVAllocator.GetCPU(a_handle);
		}
		else if constexpr (std::is_same_v<T, UAV>)
		{
			return m_UAVAllocator.GetCPU(a_handle);
		}
		else if constexpr (std::is_same_v<T, RTV>)
		{
			return m_RTVAllocator.GetCPU(a_handle);
		}
		else if constexpr (std::is_same_v<T, DSV>)
		{
			return m_DSVAllocator.GetCPU(a_handle);
		}
		//else
		//{
		//	static_assert(slways_false_v<T>, "Unsupported Heap Type");
		//	return {};
		//}
	}
	template<IsHeapType T>
	inline D3D12_GPU_DESCRIPTOR_HANDLE DescriptorHeapManager::GetGPU(Handle<T> a_handle)
	{
		if constexpr (std::is_same_v<T, CBV>)
		{
			return m_CBVAllocator.GetGPU(a_handle);
		}
		else if constexpr (std::is_same_v<T, SRV>)
		{
			return m_SRVAllocator.GetGPU(a_handle);
		}
		else if constexpr (std::is_same_v<T, UAV>)
		{
			return m_UAVAllocator.GetGPU(a_handle);
		}
		//else
		//{
		//	static_assert(slways_false_v<T>, "Unsupported Heap Type");
		//	return {};
		//}
	}
}