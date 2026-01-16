#pragma once

class Texture
{
public:

	Texture() = default;
	~Texture() = default;
	//=================================================
	// 読込
	//=================================================
	bool Load(const std::string& a_path);
	bool NormalMapLoad(const std::string& a_path);

	bool WhiteTexture();
	ID3D12Resource* GetDefaultResource(size_t a_width, size_t a_height);

	//=================================================
	// アクセサ
	//=================================================
	ID3D12Resource* GetResource() const { return m_textureResource.Get(); }	// テクスチャリソース取得
	size_t GetWidth() const { return width; }			// 幅取得
	size_t GetHeight() const { return height; }		// 高さ取得
	size_t GetMipLevels() const { return mipLevels; }	// ミップレベル数取得
	DXGI_FORMAT GetFormat() const { return format; }	// フォーマット取得

private:

	ComPtr<ID3D12Resource> m_textureResource;			// テクスチャリソース

	// メタデータ
	size_t width = 0;
	size_t height = 0;

	size_t mipLevels = 0;

	DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;
};