#include "DSVHeap.h"

#include "Engine/Graphics/RenderingEngin/RenderingEngine.h"

DescriptorHandle DSVHeap::Register(ID3D12Resource* a_resource)
{
	size_t _count = m_currentIndex;

	if (m_maxSize <= _count)
	{
		assert(0 && "DSVHeapのヒープ領域を使い切りました");
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

	// DSVの仕様書作成
	auto _device = RenderingEngine::Instance().GetDevice();
	auto _resource = a_resource;

	D3D12_DEPTH_STENCIL_VIEW_DESC _dsvDesc = {};
	
	// リソースの生成
	_device->CreateDepthStencilView(
		a_resource,
		nullptr,
		_handleCPU
	);
}
