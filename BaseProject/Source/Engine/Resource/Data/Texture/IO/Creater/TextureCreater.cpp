#include "TextureCreater.h"

D3D12_RESOURCE_DESC Engine::Resource::BuildTextureResourceDesc(const TextureCreateDesc& a_desc)
{
	// リソースの仕様書作成
	D3D12_RESOURCE_DESC _desc = {};
	_desc.Dimension			= D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	_desc.Width				= a_desc.width;
	_desc.Height			= a_desc.height;
	_desc.DepthOrArraySize	= 1;
	_desc.MipLevels			= static_cast<UINT16>(a_desc.mipLevel);
	_desc.Format			= a_desc.format;
	_desc.SampleDesc.Count	= a_desc.sampleCount;
	_desc.Layout			= D3D12_TEXTURE_LAYOUT_UNKNOWN;
	_desc.Flags				= GetResourceFlags(a_desc.usage);

	return _desc;
}

std::optional<D3D12_CLEAR_VALUE> Engine::Resource::BuildTextureClearValue(
	const TextureCreateDesc& a_desc,
	const D3D12_RESOURCE_DESC& a_resourceDesc
)
{
	D3D12_CLEAR_VALUE _clearValue = {};

	if (HasFlag(a_desc.usage, TextureUsage::DSV))
	{
		_clearValue.Format = DXGI_FORMAT_D32_FLOAT;
		_clearValue.DepthStencil.Depth = 1.0f;
		_clearValue.DepthStencil.Stencil = 0;

		return _clearValue;
	}

	if (HasFlag(a_desc.usage, TextureUsage::RTV))
	{
		// 既定は透明な黒。Texture::m_clearValue の初期値と必ず揃えること。
		// ここと食い違うと、実際のクリア色が生成時のクリアバリューと合わず、
		// ドライバの高速クリアが効かないうえに警告が出る
		const Math::Color _color = a_desc.opClerValue.value_or(Math::Color(0.f, 0.f, 0.f, 0.f));

		_clearValue.Format = a_resourceDesc.Format;
		_clearValue.Color[0] = _color.r;
		_clearValue.Color[1] = _color.g;
		_clearValue.Color[2] = _color.b;
		_clearValue.Color[3] = _color.a;

		return _clearValue;
	}

	// RTV でも DSV でもないならクリアバリューは渡さない
	return std::nullopt;
}
