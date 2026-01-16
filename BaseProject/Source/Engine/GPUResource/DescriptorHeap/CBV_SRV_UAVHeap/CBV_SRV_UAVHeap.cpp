#include "CBV_SRV_UAVHeap.h"

#include "Engine/Graphics/RenderingEngin/RenderingEngine.h"

bool CBV_SRV_UAVHeap::Create(
	ID3D12Device* a_pDevice, 
	D3D12_DESCRIPTOR_HEAP_TYPE a_type, 
	DirectX::XMFLOAT3 a_maxCounts,
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

	//m_currentIndex = 0;
	m_currentCounts = { 0.0f, 0.0f, 0.0f };

	// ディスクリプタヒープの仕様書作成
	D3D12_DESCRIPTOR_HEAP_DESC _desc = {};
	_desc.NodeMask = a_mask;
	_desc.Type = a_type;
	_desc.NumDescriptors = static_cast<UINT>(a_maxCounts.x + a_maxCounts.y + a_maxCounts.z);
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
	m_maxCounts = a_maxCounts;
	
	return true;
}

ID3D12DescriptorHeap* CBV_SRV_UAVHeap::GetHeap()
{
	return m_cpHeap.Get();
}

const D3D12_CPU_DESCRIPTOR_HANDLE CBV_SRV_UAVHeap::GetCPUHandle(UINT a_number) const
{
	D3D12_CPU_DESCRIPTOR_HANDLE _handle = m_cpHeap->GetCPUDescriptorHandleForHeapStart();
	_handle.ptr += m_incrementSize * a_number;
	return _handle;
}

const D3D12_GPU_DESCRIPTOR_HANDLE CBV_SRV_UAVHeap::GetGPUHandle(UINT a_number) const
{
	D3D12_GPU_DESCRIPTOR_HANDLE _handle = m_cpHeap->GetGPUDescriptorHandleForHeapStart();
	_handle.ptr += m_incrementSize * a_number;
	return _handle;
}



UINT CBV_SRV_UAVHeap::RegisterCBV(
	ID3D12Resource* a_resource,
	size_t a_size,
	D3D12_CONSTANT_BUFFER_VIEW_DESC& a_cbvDesc
)
{
	if (static_cast<UINT>(m_maxCounts.x) <= static_cast<UINT>(m_currentCounts.x))
	{
		assert(0 && "CBVHeapのヒープ領域を使い切りました");
		return {};
	}

	// ハンドルの作成
	auto _handleCPU = m_cpHeap->GetCPUDescriptorHandleForHeapStart();
	_handleCPU.ptr += m_incrementSize * static_cast<UINT>(m_currentCounts.x);
	

	// CBVの仕様書作成
	auto _resource = a_resource;

	D3D12_CONSTANT_BUFFER_VIEW_DESC _cbvDesc = {};
	_cbvDesc.BufferLocation = a_resource->GetGPUVirtualAddress();
	_cbvDesc.SizeInBytes = (a_size + 255) & ~255;
	a_cbvDesc = _cbvDesc;

	// CBVの生成
	m_pDevice->CreateConstantBufferView(
		&_cbvDesc,
		_handleCPU
	);

	return m_currentCounts.x++;
}

DescriptorHandle CBV_SRV_UAVHeap::RegisterSRV(ID3D12Resource* a_resource)
{
	//size_t _count = m_currentIndex;
	UINT _count = static_cast<UINT>(m_currentCounts.y);
	if (m_maxCounts.y <= _count)
	{
		assert(0 && "SRVHeapのヒープ領域を使い切りました");
		return {};
	}

	// ハンドルの作成
	DescriptorHandle _handle;
	auto _handleCPU = m_cpHeap->GetCPUDescriptorHandleForHeapStart();		// ディスクリプタヒープの先頭ハンドル取得
	//_handleCPU.ptr += m_incrementSize * (m_currentCounts.x + _count);				// 最初のアドレスからcount番目が/今回追加されたリソースのハンドル
	_handleCPU.ptr += m_incrementSize * (_count + static_cast<UINT>(m_maxCounts.x));				// 最初のアドレスからcount番目が今回追加されたリソースのハンドル
	auto _handleGPU = m_cpHeap->GetGPUDescriptorHandleForHeapStart();		// GPU版
	//_handleGPU.ptr += m_incrementSize * (m_currentCounts.x + _count);				// GPUが知るべき場所
	_handleGPU.ptr += m_incrementSize * (_count + static_cast<UINT>(m_maxCounts.x));				// GPUが知るべき場所

	// ハンドルの登録
	_handle.handleCPU = _handleCPU;
	_handle.handleGPU = _handleGPU;

	// SRVの仕様書作成
	auto _device = RenderingEngine::Instance().GetDevice();
	auto _resource = a_resource;

	D3D12_SHADER_RESOURCE_VIEW_DESC _srvDesc = {};
	_srvDesc.Format = a_resource->GetDesc().Format;
	_srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	_srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	_srvDesc.Texture2D.MipLevels = 1;

	// SRVの生成
	_device->CreateShaderResourceView(
		_resource,
		&_srvDesc,
		_handle.handleCPU
	);

//	++m_currentIndex;
	++m_currentCounts.y;
	return _handle;
}

DescriptorHandle CBV_SRV_UAVHeap::AllocateSRVRange(UINT a_count)
{
	UINT _startIndex = static_cast<UINT>(m_currentCounts.y);

	assert(
		(m_maxCounts.y - m_currentCounts.y) >= a_count &&
		"SRVHeapのヒープ領域を使い切りました"
	);

	// ハンドルの作成
	DescriptorHandle _handle;
	auto _handleCPU = m_cpHeap->GetCPUDescriptorHandleForHeapStart();
	_handleCPU.ptr += m_incrementSize * (_startIndex + static_cast<UINT>(m_maxCounts.x));
	auto _handleGPU = m_cpHeap->GetGPUDescriptorHandleForHeapStart();
	_handleGPU.ptr += m_incrementSize * (_startIndex + static_cast<UINT>(m_maxCounts.x));

	// ハンドルの登録
	_handle.handleCPU = _handleCPU;
	_handle.handleGPU = _handleGPU;
	m_currentCounts.y += static_cast<float>(a_count);
	return _handle;
}

DescriptorHandle CBV_SRV_UAVHeap::AllocateSRVRange(std::vector<ID3D12Resource*> a_resource)
{
	auto _handle = AllocateSRVRange(static_cast<UINT>(a_resource.size()));
	
	// SRVの仕様書作成
	auto _device = RenderingEngine::Instance().GetDevice();
	auto _resource = a_resource;
	//for (auto& _res : a_resource)
	for (UINT _i = 0; _i < a_resource.size(); ++_i)
	{
		auto& _res = a_resource[_i];

		if (_res == nullptr)
		{
			continue;
		}

		D3D12_SHADER_RESOURCE_VIEW_DESC _srvDesc = {};
		_srvDesc.Format = _res->GetDesc().Format;
		_srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		_srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		_srvDesc.Texture2D.MipLevels = 1;

		_handle.handleCPU.ptr = 
			_handle.handleCPU.ptr + static_cast<unsigned int>(m_incrementSize * _i);

		// SRVの生成
		_device->CreateShaderResourceView(
			_res,
			&_srvDesc,
			_handle.handleCPU
		);
	}

	return _handle;
}

DescriptorHandle CBV_SRV_UAVHeap::RegisterUAV(ID3D12Resource* a_resource)
{
	//size_t _count = m_currentIndex;
	UINT _count = static_cast<UINT>(m_currentCounts.z);

	if (m_maxCounts.z <= _count)
	{
		assert(0 && "CBV_SRV_UAVHeapのヒープ領域を使い切りました");
		return {};
	}
	return DescriptorHandle();
}
