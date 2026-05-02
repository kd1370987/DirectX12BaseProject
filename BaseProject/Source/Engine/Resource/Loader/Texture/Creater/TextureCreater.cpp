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
	D3D12_CLEAR_VALUE clearValue = {};
	D3D12_CLEAR_VALUE* _pClear = nullptr;

	if (HasFlag(a_desc.usage, TextureUsage::DSV))
	{
		clearValue.Format = DXGI_FORMAT_D32_FLOAT;
		clearValue.DepthStencil.Depth = 1.0f;
		clearValue.DepthStencil.Stencil = 0;
		_pClear = &clearValue;
	}
	else if(HasFlag(a_desc.usage,TextureUsage::RTV))
	{
		if (a_desc.opClerValue.has_value())
		{
			DXSM::Color _color = a_desc.opClerValue.value();
			clearValue.Format = _desc.Format;
			clearValue.Color[0] = _color.R();
			clearValue.Color[1] = _color.G();
			clearValue.Color[2] = _color.B();
			clearValue.Color[3] = _color.A();
		}
		else
		{
			clearValue.Format = _desc.Format;
			clearValue.Color[0] = 0;
			clearValue.Color[1] = 0;
			clearValue.Color[2] = 0;
			clearValue.Color[3] = 1;
		}
		_pClear = &clearValue;
	}

	// リソース作成
	HRESULT _hr = Engine::D3D12::D3D12Wrapper::Instance().GetDevice()->CreateCommittedResource(
		&_heapProp,
		D3D12_HEAP_FLAG_NONE,
		&_desc,
		D3D12_RESOURCE_STATE_COMMON,
		_pClear,
		IID_PPV_ARGS(_cpResource.ReleaseAndGetAddressOf())
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
