#include "DescriptorHeap.h"

bool DescriptorHeap::Create(
	ID3D12Device* a_pDevice,
	D3D12_DESCRIPTOR_HEAP_TYPE a_type, 
	UINT a_numDescriptors,
	D3D12_DESCRIPTOR_HEAP_FLAGS a_flags,
	UINT a_mask
)
{
	if (a_pDevice == nullptr)
	{
		assert(0 && "デバイスがnullptrです\n");
		return false;
	}
	m_pDevice = a_pDevice;

	m_currentIndex = 0;

	// ディスクリプタヒープの仕様書作成
	D3D12_DESCRIPTOR_HEAP_DESC _desc = {};
	_desc.NodeMask = a_mask;
	_desc.Type = a_type;
	_desc.NumDescriptors = a_numDescriptors;
	_desc.Flags = a_flags;

	// ディスクリプタヒープの生成
	auto _hr = m_pDevice->CreateDescriptorHeap(
		&_desc,
		IID_PPV_ARGS(m_cpHeap.ReleaseAndGetAddressOf())
	);
	if (FAILED(_hr))
	{
		assert(0 && "ディスクリプタヒープ作成失敗\n");
		return false;
	}

	// インクリメントサイズの取得
	m_incrementSize = m_pDevice->GetDescriptorHandleIncrementSize(_desc.Type);
	m_type = a_type;
	m_maxSize = a_numDescriptors;
	
	return true;
}

ID3D12DescriptorHeap* DescriptorHeap::GetHeap()
{
	return m_cpHeap.Get();
}

const D3D12_CPU_DESCRIPTOR_HANDLE DescriptorHeap::GetCPUHandle(UINT a_number) const
{
	D3D12_CPU_DESCRIPTOR_HANDLE _handle = m_cpHeap->GetCPUDescriptorHandleForHeapStart();
	_handle.ptr += m_incrementSize * a_number;
	return _handle;
}

const D3D12_GPU_DESCRIPTOR_HANDLE DescriptorHeap::GetGPUHandle(UINT a_number) const
{
	D3D12_GPU_DESCRIPTOR_HANDLE _handle = m_cpHeap->GetGPUDescriptorHandleForHeapStart();
	_handle.ptr += m_incrementSize * a_number;
	return _handle;
}
