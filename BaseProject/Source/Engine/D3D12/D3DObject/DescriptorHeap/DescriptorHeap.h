#pragma once

namespace Engine::D3D12
{
	template<D3D12_DESCRIPTOR_HEAP_TYPE HeapType>
	class DescriptorHeap
	{
	public:

		// 生成
		bool Create(
			ID3D12Device* a_pDevice,
			UINT a_maxCount,
			D3D12_DESCRIPTOR_HEAP_FLAGS a_flags,
			UINT a_mask
		);

		// ハンドル確保
		D3D12_CPU_DESCRIPTOR_HANDLE GetCPU(UINT a_index);
		D3D12_GPU_DESCRIPTOR_HANDLE GetGPU(UINT a_index);

		// ヒープの生ポインタ取得
		ID3D12DescriptorHeap* GetHeap() const;

		// ヒープサイズ確保
		UINT GetMaxSize();

	private:

		ID3D12Device* m_pDevice = nullptr;					// デバイスポインタ

		ComPtr<ID3D12DescriptorHeap> m_cpHeap = nullptr;	// ヒープ
		UINT m_incrementSize = 0;							// ヒープのインクリメントサイズ

		// 確保ヒープサイズ
		UINT m_maxSize = 0;
	};

	template<D3D12_DESCRIPTOR_HEAP_TYPE HeapType>
	inline bool DescriptorHeap<HeapType>::Create(
		ID3D12Device* a_pDevice,
		UINT a_maxCount,
		D3D12_DESCRIPTOR_HEAP_FLAGS a_flags,
		UINT a_mask
	)
	{
		// デバイスチェック
		m_pDevice = a_pDevice;
		if (!m_pDevice)
		{
			assert(0 && "デバイスがありません");
			return false;
		}

		// ディスクリプタヒープの仕様書作成
		D3D12_DESCRIPTOR_HEAP_DESC _desc = {};
		_desc.NodeMask = a_mask;
		_desc.Type = HeapType;
		_desc.NumDescriptors = a_maxCount;
		_desc.Flags = a_flags;

		// ディスクリプタヒープの生成
		HRESULT _hr = m_pDevice->CreateDescriptorHeap(
			&_desc,
			IID_PPV_ARGS(m_cpHeap.ReleaseAndGetAddressOf())
		);
		if (FAILED(_hr))
		{
			assert(0 && "ディスクリプタヒープ作成失敗");
			return false;
		}

		// インクリメントサイズの取得
		m_incrementSize = m_pDevice->GetDescriptorHandleIncrementSize(HeapType);
		m_maxSize = a_maxCount;
		return true;
	}
	template<D3D12_DESCRIPTOR_HEAP_TYPE HeapType>
	inline D3D12_CPU_DESCRIPTOR_HANDLE DescriptorHeap<HeapType>::GetCPU(
		UINT a_index
	)
	{
		D3D12_CPU_DESCRIPTOR_HANDLE _handle = m_cpHeap->GetCPUDescriptorHandleForHeapStart();
		_handle.ptr += m_incrementSize * static_cast<UINT>(a_index);
		return _handle;
	}
	template<D3D12_DESCRIPTOR_HEAP_TYPE HeapType>
	inline D3D12_GPU_DESCRIPTOR_HANDLE DescriptorHeap<HeapType>::GetGPU(
		UINT a_index
	)
	{
		D3D12_GPU_DESCRIPTOR_HANDLE _handle = m_cpHeap->GetGPUDescriptorHandleForHeapStart();
		_handle.ptr += m_incrementSize * static_cast<UINT>(a_index);
		return _handle;
	}
	template<D3D12_DESCRIPTOR_HEAP_TYPE HeapType>
	inline ID3D12DescriptorHeap* DescriptorHeap<HeapType>::GetHeap() const
	{
		return m_cpHeap.Get();
	}
	template<D3D12_DESCRIPTOR_HEAP_TYPE HeapType>
	inline UINT DescriptorHeap<HeapType>::GetMaxSize()
	{
		return m_maxSize;
	}
}

