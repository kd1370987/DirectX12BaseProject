#include "RenderTarget.h"

void RenderTarget::Reset()
{
	m_cpResource.Reset();
}

bool RenderTarget::Create(ID3D12Device* a_pDevice)
{
	// 仕様書作成
	D3D12_RESOURCE_DESC _resDesc = {};
	if (m_upResourceDesc)
	{
		// 仕様書が指定されているのならそれを使用
		_resDesc = *m_upResourceDesc.get();
	}
	else
	{
		// 仕様書が指定されていないのなら作成
	}

	// ヒーププロパティの作成
	D3D12_HEAP_PROPERTIES _heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

	// クリアバリューの作成
	float _clsClr[4] = { 0.0f,0.0f,0.0f,1.0f };
	D3D12_CLEAR_VALUE _clearValue = CD3DX12_CLEAR_VALUE(DXGI_FORMAT_R8G8B8A8_UNORM, _clsClr);

	HRESULT _hr = a_pDevice->CreateCommittedResource(
		&_heapProp,
		D3D12_HEAP_FLAG_NONE,
		&_resDesc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		&_clearValue,
		IID_PPV_ARGS(m_cpResource.ReleaseAndGetAddressOf())
	);
	if (FAILED(_hr))
	{
		assert(0 && "レンダーターゲットの作成に失敗");
		return false;
	}

}

void RenderTarget::Create(IDXGISwapChain* a_pSwapChain, UINT a_bufferIndex)
{
	// スワップチェインからバックバッファを生成
	a_pSwapChain->GetBuffer(
		a_bufferIndex,
		IID_PPV_ARGS(m_cpResource.ReleaseAndGetAddressOf())
	);
}


void RenderTarget::SetResourceDesc(const D3D12_RESOURCE_DESC& a_desc)
{
	m_upResourceDesc = std::make_unique<D3D12_RESOURCE_DESC>(a_desc);
}
