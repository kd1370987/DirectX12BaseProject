#pragma once
namespace Engine::Resource
{
	struct TextureDesc
	{
		ComPtr<ID3D12Resource> cpResource = nullptr;
		D3D12_RESOURCE_DESC desc = {};
	};

	class Texture;

	// 更新用バッファ
	struct UploadBuffer
	{
		ID3D12Resource* pResource = nullptr;

		UINT subresourceCount = 0;

		std::vector<D3D12_PLACED_SUBRESOURCE_FOOTPRINT> layoutVec;
		std::vector<UINT> numRowVec;
		std::vector<UINT64> rowSizeVec;
	};

	// テクスチャ読み込み
	TextureDesc ImportTexture(
		const std::string& a_filePath,
		D3D12_RESOURCE_DESC* a_desc = nullptr
	);

	// 色を指定してデフォルトテクスチャ生成
	TextureDesc DefaultTexture(DirectX::XMFLOAT4 a_color);

	// 白テクスチャ
	TextureDesc WhiteTexture();

	// 黒テクスチャ
	TextureDesc BlackTexture();

	// ノーマルマップ白テクスチャ
	TextureDesc NormalWhiteTexture();

	// ORMテクスチャ
	TextureDesc ORMTexture();
}