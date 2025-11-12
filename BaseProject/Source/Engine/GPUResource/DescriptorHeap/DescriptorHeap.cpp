#include "DescriptorHeap.h"

#include "Engine/Graphics/RenderingEngin/RenderingEngine.h"
#include "Engine/GPUResource/Texture/Texture2D/Texture2D.h"

const UINT HANDLE_MAX = 512;

DescriptorHeap::DescriptorHeap()
{
	m_pHandles.clear();					// クリア
	m_pHandles.reserve(HANDLE_MAX);		// メモリ確保

	// ディスクリプタヒープの仕様書作成
	D3D12_DESCRIPTOR_HEAP_DESC _desc = {};
	_desc.NodeMask = 1;
	_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	_desc.NumDescriptors = HANDLE_MAX;
	_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

	// デバイスの取得
	auto _device = RenderingEngine::Instance().GetDevice();

	// ディスクリプタヒープの生成
	auto _hr = _device->CreateDescriptorHeap(
		&_desc, 
		IID_PPV_ARGS(m_pHeap.ReleaseAndGetAddressOf())
	);
	if (FAILED(_hr))
	{
		m_isValid = false;
		return;
	}

	m_incrementSize = _device->GetDescriptorHandleIncrementSize(_desc.Type);
	m_isValid = true;

	printf("ディスクリプタヒープの生成に成功\n");
}

ID3D12DescriptorHeap* DescriptorHeap::GetHeap()
{
	return m_pHeap.Get();
}

DescriptorHandle* DescriptorHeap::Register(Texture2D* a_texture)
{
	auto _count = m_pHandles.size();
	if (HANDLE_MAX <= _count)
	{
		return nullptr;
	}

	// ハンドルの作成
	DescriptorHandle* _pHandle = new DescriptorHandle();

	auto _handleCPU = m_pHeap->GetCPUDescriptorHandleForHeapStart();		// ディスクリプタヒープの先頭ハンドル取得
	_handleCPU.ptr += m_incrementSize * _count;			// 最初のアドレスからcount番目が今回追加されたリソースのハンドル

	auto _handleGPU = m_pHeap->GetGPUDescriptorHandleForHeapStart();		// ディスクリプタヒープの先頭ハンドル取得
	_handleGPU.ptr += m_incrementSize * _count;			// 最初のアドレスからcount番目が今回追加されたリソースのハンドル

	// ハンドルの登録
	_pHandle->HandoleCPU = _handleCPU;
	_pHandle->HandoleGPU = _handleGPU;

	auto _device = RenderingEngine::Instance().GetDevice();
	auto _resource = a_texture->Resource();
	auto _desc = a_texture->ViewDesc();

	// SRVの生成
	_device->CreateShaderResourceView(
		_resource,
		&_desc,
		_pHandle->HandoleCPU
	);

	// ハンドルリストに追加
	m_pHandles.push_back(_pHandle);

	// ハンドルを返す
	return _pHandle;
}
