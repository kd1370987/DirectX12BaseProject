#include "RTVHeap.h"

#include "Engine/D3D12/D3D12Wrapper/D3D12Wrapper.h"

RTVHandle RTVHeap::RegisterRTV(ID3D12Resource* a_resource, D3D12_RENDER_TARGET_VIEW_DESC* a_pRtvDesc)
{
	// RTVの位置を取得
	RTVHandle _handle = {};
	if (m_indexQueue.empty())
	{
		assert(0 && "RTVの使用上限に達しました");
		_handle.index = INVALID_INDEX;
		return _handle;
	}
	_handle = m_indexQueue.front();
	m_indexQueue.pop();


	// ハンドルの作成
	auto _handleCPU = m_cpHeap->GetCPUDescriptorHandleForHeapStart();
	_handleCPU.ptr += m_incrementSize * _handle.index;

	// RTVの生成
	m_pDevice->CreateRenderTargetView(
		a_resource,
		a_pRtvDesc,
		_handleCPU
	);

	return _handle;
}

bool RTVHeap::Create(
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
	// デバイスのポインタを登録
	m_pDevice = a_pDevice;

	// インデックスの待ち行列作成
	for (UINT _idx = 0; _idx < a_numDescriptors; ++_idx)
	{
		RTVHandle _handle;
		_handle.index = _idx;
		m_indexQueue.push(_handle);
	}

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

	return true;
}

ID3D12DescriptorHeap* RTVHeap::GetHeap()
{
	return m_cpHeap.Get();
}

const D3D12_CPU_DESCRIPTOR_HANDLE RTVHeap::GetCPUHandle(RTVHandle a_handleIndex) const
{
	D3D12_CPU_DESCRIPTOR_HANDLE _handle = m_cpHeap->GetCPUDescriptorHandleForHeapStart();
	_handle.ptr += m_incrementSize * a_handleIndex.index;
	return _handle;
}
