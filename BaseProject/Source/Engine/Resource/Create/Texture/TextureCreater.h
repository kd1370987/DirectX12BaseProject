#pragma once
namespace Engine::Resource
{
	// テクスチャ生成設定
	struct TextureCreateDesc
	{
		UINT64 width = 0;
		UINT height = 0;

		DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;

		UINT mipLevel = 1;
		UINT sampleCount = 1;

		// テクスチャの使用方法
		TextureUsage usage = TextureUsage::None;
	};

	// テクスチャ作成
	ComPtr<ID3D12Resource> CreateTexture(
		const TextureCreateDesc& a_desc,
		D3D12_RESOURCE_DESC* a_pOutDesc = nullptr
	);
}