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

		// 解放
		void Release();

		// ビュー操作
		Handle<T> Allocate(D3D12::Device* a_pDevice,ID3D12Resource* a_pRes,const typename T::DescType* a_desc);

		// ビュー消去
		void Remove(Handle<T> a_handle);

		// ハンドル取得
		D3D12_CPU_DESCRIPTOR_HANDLE GetCPU(const Handle<T>& a_handle) const;
		D3D12_GPU_DESCRIPTOR_HANDLE GetGPU(const Handle<T>& a_handle) const;

	private:
		// 参照元ヒープ
		DescriptorHeap<T::type>* m_pHeap = nullptr;
		
		// ハンドル管理
		Storage::HandlePool<T> m_HandlePool = {};

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
			m_HandlePool.Create(a_pHeap->GetMaxSize());		// ハンドル管理
		}
		else
		{
			m_maxCount = a_maxCount;
			m_HandlePool.Create(a_maxCount);					// ハンドル管理
		}
		
		return true;
	}
	template<IsHeapType T>
	inline void HeapAllocator<T>::Release()
	{
		m_pHeap = nullptr;
	}
	template<IsHeapType T>
	inline Handle<T> HeapAllocator<T>::Allocate(D3D12::Device* a_pDevice, ID3D12Resource* a_pRes, const typename T::DescType* a_desc)
	{
		// ハンドルをアロケート
		auto _handle = m_HandlePool.Allocate();
		auto _idx = _handle.GetIndex();
		_handle.SetIndex(_idx + (uint16_t)m_startIndex);

		// CPUハンドルを取得する処理
		D3D12_CPU_DESCRIPTOR_HANDLE _cpuHandle = m_pHeap->GetCPU(static_cast<UINT>(_handle.GetIndex()));

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
	inline void HeapAllocator<T>::Remove(Handle<T> a_handle)
	{
		// ハンドルが有効じゃなければ返す
		if (Handle<T>() == a_handle) return;

		// 戻してあげる
		auto _handle = a_handle;
		auto _idx = _handle.GetIndex();
		_handle.SetIndex(_idx - m_startIndex);
		m_HandlePool.Remove(_handle);
	}
	template<IsHeapType T>
	inline D3D12_CPU_DESCRIPTOR_HANDLE HeapAllocator<T>::GetCPU(const Handle<T>&a_handle) const
	{
		auto _stHandle = a_handle;
		auto _idx = _stHandle.GetIndex();
		_stHandle.SetIndex(_idx - m_startIndex);
		if (m_HandlePool.IsValid(_stHandle))
		{
			UINT _idx = a_handle.GetIndex();
			return m_pHeap->GetCPU(_idx);
		}
		return { 0 };
	}
	template<IsHeapType T>
	inline D3D12_GPU_DESCRIPTOR_HANDLE HeapAllocator<T>::GetGPU(const Handle<T>& a_handle) const
	{
		auto _stHandle = a_handle;
		auto _idx = _stHandle.GetIndex();
		_stHandle.SetIndex(_idx - m_startIndex);
		if (m_HandlePool.IsValid(_stHandle))
		{
			UINT _idx = a_handle.GetIndex();
			return m_pHeap->GetGPU(_idx);
		}
		return { 0 };
	}
}