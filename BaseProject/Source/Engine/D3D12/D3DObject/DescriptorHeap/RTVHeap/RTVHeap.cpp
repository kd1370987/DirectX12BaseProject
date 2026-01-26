#include "RTVHeap.h"

#include "Engine/D3D12/D3D12Wrapper/RenderingEngine.h"

DescriptorHandle RTVHeap::Register(ID3D12Resource* a_resource)
{
	size_t _count = m_currentIndex;

	if (m_maxSize <= _count)
	{
		assert(0 && "RTHeapのヒープ領域を使い切りました");
		return {};
	}

	// ハンドルの作成
	DescriptorHandle _handle;
	auto _handleCPU = m_cpHeap->GetCPUDescriptorHandleForHeapStart();		// ディスクリプタヒープの先頭ハンドル取得
	_handleCPU.ptr += m_incrementSize * _count;				// 最初のアドレスからcount番目が今回追加されたリソースのハンドル
	auto _handleGPU = m_cpHeap->GetGPUDescriptorHandleForHeapStart();		// GPU版
	_handleGPU.ptr += m_incrementSize * _count;				// GPUが知るべき場所

	// ハンドルの登録
	_handle.handleCPU = _handleCPU;
	_handle.handleGPU = _handleGPU;

	// RTVヒープ仕様書作成
	auto _device = RenderingEngine::Instance().GetDevice();
	auto _resource = a_resource;

	// RTVの生成
	_device->CreateRenderTargetView(
		a_resource,
		nullptr,
		_handleCPU
	);

	++m_currentIndex;
	return _handle;

}

UINT RTVHeap::RegisterRTV(ID3D12Resource* a_resource, D3D12_RENDER_TARGET_VIEW_DESC* a_pRtvDesc)
{
	// RTVの位置を取得
	if (m_indexQueue.empty())
	{
		assert(0 && "RTVの使用上限に達しました");
		return 9999;
	}
	UINT _count = m_indexQueue.front();
	m_indexQueue.pop();


	// ハンドルの作成
	auto _handleCPU = m_cpHeap->GetCPUDescriptorHandleForHeapStart();
	_handleCPU.ptr += m_incrementSize * _count;

	// RTVの生成
	m_pDevice->CreateRenderTargetView(
		a_resource,
		a_pRtvDesc,
		_handleCPU
	);

	return _count;
}

bool RTVHeap::Create(ID3D12Device* a_pDevice, D3D12_DESCRIPTOR_HEAP_TYPE a_type, UINT a_numDescriptors, D3D12_DESCRIPTOR_HEAP_FLAGS a_flags, UINT a_mask)
{
	if (a_pDevice == nullptr)
	{
		assert(0 && "デバイスがnullptrです\n");
		return false;
	}

	for (UINT _idx = 0; _idx < a_numDescriptors; ++_idx)
	{
		m_indexQueue.push(_idx);
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

ID3D12DescriptorHeap* RTVHeap::GetHeap()
{
	return m_cpHeap.Get();
}

const D3D12_CPU_DESCRIPTOR_HANDLE RTVHeap::GetCPUHandle(UINT a_number) const
{
	D3D12_CPU_DESCRIPTOR_HANDLE _handle = m_cpHeap->GetCPUDescriptorHandleForHeapStart();
	_handle.ptr += m_incrementSize * a_number;
	return _handle;
}

const D3D12_GPU_DESCRIPTOR_HANDLE RTVHeap::GetGPUHandle(UINT a_number) const
{
	D3D12_GPU_DESCRIPTOR_HANDLE _handle = m_cpHeap->GetGPUDescriptorHandleForHeapStart();
	_handle.ptr += m_incrementSize * a_number;
	return _handle;
}
