#include "TextureCreater.h"

#include "Engine/D3D12/D3D12Wrapper/D3D12Wrapper.h"


ComPtr<ID3D12Resource> Engine::Resource::CreateTexture(
	const TextureCreateDesc& a_desc,
	D3D12_RESOURCE_DESC* a_pOutDesc
)
{
	// 空リソース生成
	ComPtr<ID3D12Resource> _cpResource = nullptr;

	// リソースの仕様書作成
	D3D12_RESOURCE_DESC _desc = {};
	_desc.Dimension			= D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	_desc.Width				= a_desc.width;
	_desc.Height			= a_desc.height;
	_desc.DepthOrArraySize	= 1;
	_desc.MipLevels			= a_desc.mipLevel;
	_desc.Format			= a_desc.format;
	_desc.SampleDesc.Count	= a_desc.sampleCount;
	_desc.Layout			= D3D12_TEXTURE_LAYOUT_UNKNOWN;
	_desc.Flags				= GetResourceFlags(a_desc.usage);

	// ヒープ作成
	auto _heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

	// クリアバリュー作成
	D3D12_CLEAR_VALUE* _pClear = nullptr;
	if (HasFlag(a_desc.usage, TextureUsage::DSV))
	{
		_pClear = new(D3D12_CLEAR_VALUE);
		_pClear->Format = DXGI_FORMAT_D32_FLOAT;
		_pClear->DepthStencil.Depth = static_cast<FLOAT>(1.0f);
		_pClear->DepthStencil.Stencil= static_cast<UINT8>(0.0f);
	}
	else
	{
		_pClear = new(D3D12_CLEAR_VALUE);
		_pClear->Format = _desc.Format;
		_pClear->Color[0] = 0;
		_pClear->Color[1] = 0;
		_pClear->Color[2] = 0;
		_pClear->Color[3] = 1;
	}

	// リソース作成
	HRESULT _hr = D3D12Wrapper::Instance().GetDevice()->CreateCommittedResource(
		&_heapProp,
		D3D12_HEAP_FLAG_NONE,
		&_desc,
		D3D12_RESOURCE_STATE_COMMON,
		_pClear,
		IID_PPV_ARGS(&_cpResource)
	);
	if (FAILED(_hr))
	{
		assert(0 && "Textureの生成に失敗");
		return _cpResource;
	}
	if (a_pOutDesc)
	{
		*a_pOutDesc = _desc;
	}

	return _cpResource;
}
