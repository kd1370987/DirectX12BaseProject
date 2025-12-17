#include "DescriptorHeap.h"

#include "Engine/Graphics/RenderingEngin/RenderingEngine.h"

bool DescriptorHeap::Create(D3D12_DESCRIPTOR_HEAP_TYPE a_type, UINT a_numDescriptors, D3D12_DESCRIPTOR_HEAP_FLAGS a_flags, UINT a_mask)
{
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
		IID_PPV_ARGS(m_cpHeap.ReleaseAndGetAddressOf())
	);
	if (FAILED(_hr))
	{
		printf("ディスクリプタヒープ作成失敗\n");
		return false;
	}

	// インクリメントサイズの取得
	m_incrementSize = _device->GetDescriptorHandleIncrementSize(_desc.Type);
	m_type = a_type;
	m_maxSize = a_numDescriptors;
	printf("ディスクリプタヒープ作成成功\n");
	return true;
}

ID3D12DescriptorHeap* DescriptorHeap::GetHeap()
{
	return m_cpHeap.Get();
}
