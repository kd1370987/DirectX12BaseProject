#include "SRVHeap.h"

#include "Engine/Graphics/RenderingEngin/RenderingEngine.h"

DescriptorHandle SRVHeap::Register(ID3D12Resource* a_resource)
{
	size_t _count = m_currentIndex;

	if (m_maxSize <= _count)
	{
		assert(0 && "SRVHeapのヒープ領域を使い切りました");
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

	++m_currentIndex;
	return _handle;
}
