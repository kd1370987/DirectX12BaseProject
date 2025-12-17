#include "CBVHeap.h"
#include "Engine/Graphics/RenderingEngin/RenderingEngine.h"

DescriptorHandle CBVHeap::Register(ID3D12Resource* a_resource)
{
	size_t _count = m_currentIndex;
	if (m_maxSize <= _count)
	{
		assert(0 && "CBVHeapのヒープ領域を使い切りました");
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

	// CBVの仕様書作成
	auto _device = RenderingEngine::Instance().GetDevice();
	auto _resource = a_resource;

	D3D12_CONSTANT_BUFFER_VIEW_DESC _cbvDesc = {};
	_cbvDesc.BufferLocation = a_resource->GetGPUVirtualAddress();
	_cbvDesc.SizeInBytes = (a_resource->GetDesc().Width + 255) & ~255;
	

	// CBVの生成
	_device->CreateConstantBufferView(
		&_cbvDesc,
		_handle.handleCPU
	);

	++m_currentIndex;
	return _handle;
}
