#pragma once
namespace Engine::Resource
{
	// テクスチャ作成
	ComPtr<ID3D12Resource> CreateTexture(
		const TextureCreateDesc& a_desc,
		D3D12_RESOURCE_DESC* a_pOutDesc = nullptr
	);
}