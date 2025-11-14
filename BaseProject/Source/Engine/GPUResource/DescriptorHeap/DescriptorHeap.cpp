#include "DescriptorHeap.h"

#include "Engine/Graphics/RenderingEngin/RenderingEngine.h"

bool DescriptorHeap::Create(D3D12_DESCRIPTOR_HEAP_TYPE a_type, UINT a_numDescriptors, D3D12_DESCRIPTOR_HEAP_FLAGS a_flags, UINT a_mask)
{
	//m_handles.clear();							// クリア
	//m_handles.reserve(a_numDescriptors);		// メモリ確保
	m_currentIndex = 0;

	// ディスクリプタヒープの仕様書作成
	D3D12_DESCRIPTOR_HEAP_DESC _desc = {};
	_desc.NodeMask = a_mask;
	_desc.Type = a_type;
	_desc.NumDescriptors = a_numDescriptors;
	_desc.Flags = a_flags;

	// デバイスの取得
	auto _device = RenderingEngine::Instance().GetDevice();

	// ディスクリプタヒープの生成
	auto _hr = _device->CreateDescriptorHeap(
		&_desc,
		IID_PPV_ARGS(m_pHeap.ReleaseAndGetAddressOf())
	);
	if (FAILED(_hr))
	{
		return false;
	}

	// インクリメントサイズの取得
	m_incrementSize = _device->GetDescriptorHandleIncrementSize(_desc.Type);
	m_type = a_type;
	m_maxSize = a_numDescriptors;
	return true;
}

ID3D12DescriptorHeap* DescriptorHeap::GetHeap()
{
	return m_pHeap.Get();
}

DescriptorHandle DescriptorHeap::RegisterCPUOnly()
{
	return DescriptorHandle();
}

DescriptorHandle DescriptorHeap::RegisterSRV(ID3D12Resource* a_resource)
{
	//auto _count = m_handles.size();
	size_t _count = m_currentIndex;
	if (m_maxSize <= _count)
	{
		return {};
	}
	if (m_type != D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)
	{
		return {};
	}

	//==============================================================
	// ハンドルの作成
	//==============================================================
	DescriptorHandle _handle;
	auto _handleCPU = m_pHeap->GetCPUDescriptorHandleForHeapStart();		// ディスクリプタヒープの先頭ハンドル取得
	_handleCPU.ptr += m_incrementSize * _count;			// 最初のアドレスからcount番目が今回追加されたリソースのハンドル
	auto _handleGPU = m_pHeap->GetGPUDescriptorHandleForHeapStart();		// ディスクリプタヒープの先頭ハンドル取得
	_handleGPU.ptr += m_incrementSize * _count;			// 最初のアドレスからcount番目が今回追加されたリソースのハンドル

	//==============================================================
	// ハンドルの登録
	//==============================================================
	_handle.handleCPU = _handleCPU;
	_handle.handleGPU = _handleGPU;

	auto _device = RenderingEngine::Instance().GetDevice();
	auto _resource = a_resource;
	D3D12_SHADER_RESOURCE_VIEW_DESC _srvDesc = {};
	_srvDesc.Format = a_resource->GetDesc().Format;									// フォーマット

	// SRVの生成
	_device->CreateShaderResourceView(
		_resource,
		&_srvDesc,
		_handle.handleCPU
	);

	// ハンドルリストに追加
	//m_handles.push_back(_handle);
	m_currentIndex++;

	// ハンドルを返す
	return _handle;
}

DescriptorHandle DescriptorHeap::RegisterUAV(ID3D12Resource* a_resource)
{
	return DescriptorHandle();
}

DescriptorHandle DescriptorHeap::RegisterSampler()
{
	return DescriptorHandle();
}

