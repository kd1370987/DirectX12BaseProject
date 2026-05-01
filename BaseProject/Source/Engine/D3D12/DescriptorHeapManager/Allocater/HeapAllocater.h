#pragma once
namespace Engine::D3D12
{
	// 制約(IsHeapType)を満たす型のTのみを受け付ける
	template<IsHeapType T>
	class HeapAllocator
	{
	public:
		// アロケーター作成
		bool Create(DescriptorHeap<T::type>* a_pHeap, UINT a_startIdx = 0, UINT a_maxCount = 0);

		// ビュー操作
		Resource::Handle<T> Allocate(ID3D12Device* a_pDevice,ID3D12Resource* a_pRes,const typename T::DescType* a_desc);

		// ビュー消去
		void Remove(Resource::Handle<T> a_handle);

		// ハンドル取得
		D3D12_CPU_DESCRIPTOR_HANDLE GetCPU(const Resource::Handle<T>& a_handle) const;
		D3D12_GPU_DESCRIPTOR_HANDLE GetGPU(const Resource::Handle<T>& a_handle) const;

	private:
		// 参照元ヒープ
		DescriptorHeap<T::type>* m_pHeap = nullptr;
		
		// ハンドル管理
		Storage::HandleStorage<T> m_handleStorage = {};

		UINT m_startIndex = 0;
		UINT m_maxCount = 0;
	};

	template<IsHeapType T>
	inline bool HeapAllocator<T>::Create(DescriptorHeap<T::type>* a_pHeap, UINT a_startIdx, UINT a_maxCount)
	{
		// ヒープの参照
		m_pHeap = a_pHeap;

		m_startIndex = a_startIdx;
		if (a_maxCount == 0)
		{
			m_maxCount = a_pHeap->GetMaxSize();
			m_handleStorage.Create(a_pHeap->GetMaxSize());		// ハンドル管理
		}
		else
		{
			m_maxCount = a_maxCount;
			m_handleStorage.Create(a_maxCount);					// ハンドル管理
		}
		
		return true;
	}
	template<IsHeapType T>
	inline Resource::Handle<T> HeapAllocator<T>::Allocate(ID3D12Device* a_pDevice, ID3D12Resource* a_pRes, const typename T::DescType* a_desc)
	{
		// ハンドルをアロケート
		auto _handle = m_handleStorage.Allocate();

		// CPUハンドルを取得する処理
		D3D12_CPU_DESCRIPTOR_HANDLE _cpuHandle = m_pHeap->GetCPU(m_startIndex + static_cast<UINT>(_handle.idx));

		// ビュー作成
		if constexpr (std::is_same_v<T, CBV>)
		{
			a_pDevice->CreateConstantBufferView(a_desc,_cpuHandle);
		}
		else if constexpr (std::is_same_v<T, SRV>)
		{
			D3D12_SHADER_RESOURCE_VIEW_DESC _srvDesc = {};
			if (a_desc)
			{
				_srvDesc = *a_desc;
			}
			else
			{
				_srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
				_srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
				_srvDesc.Texture2D.MipLevels = a_pRes->GetDesc().MipLevels;
			}

			a_pDevice->CreateShaderResourceView(a_pRes, &_srvDesc, _cpuHandle);
		}
		else if constexpr (std::is_same_v<T, UAV>) 
		{
			D3D12_UNORDERED_ACCESS_VIEW_DESC _UAVDesc = {};
			if (a_desc)
			{
				_UAVDesc = *a_desc;
			}
			else
			{
				_UAVDesc.Format = a_pRes->GetDesc().Format;
				_UAVDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
				_UAVDesc.Texture2D.MipSlice = 0;
			}

			a_pDevice->CreateUnorderedAccessView(a_pRes, nullptr, &_UAVDesc, _cpuHandle);
		}
		else if constexpr (std::is_same_v<T, RTV>) 
		{
			a_pDevice->CreateRenderTargetView(a_pRes, a_desc, _cpuHandle);
		}
		else if constexpr (std::is_same_v<T, DSV>) 
		{
			a_pDevice->CreateDepthStencilView(a_pRes, a_desc, _cpuHandle);
		}

		// ハンドルを返す
		return _handle;
	}
	template<IsHeapType T>
	inline void HeapAllocator<T>::Remove(Resource::Handle<T> a_handle)
	{
		m_handleStorage.Remove(a_handle);
	}
	template<IsHeapType T>
	inline D3D12_CPU_DESCRIPTOR_HANDLE HeapAllocator<T>::GetCPU(const Resource::Handle<T>&a_handle) const
	{
		if (m_handleStorage.IsValid(a_handle))
		{
			UINT _idx = m_startIndex + a_handle.idx;
			return m_pHeap->GetCPU(_idx);
		}
		return { 0 };
	}
	template<IsHeapType T>
	inline D3D12_GPU_DESCRIPTOR_HANDLE HeapAllocator<T>::GetGPU(const Resource::Handle<T>& a_handle) const
	{
		if (m_handleStorage.IsValid(a_handle))
		{
			UINT _idx = m_startIndex + a_handle.idx;
			return m_pHeap->GetGPU(_idx);
		}
		return { 0 };
	}
}