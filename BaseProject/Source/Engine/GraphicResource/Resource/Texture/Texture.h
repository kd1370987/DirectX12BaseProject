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
	//ID3D12Resource* GetDefaultResource(size_t a_width, size_t a_height);

	//=================================================
	// アクセサ
	//=================================================
	ID3D12Resource* GetResource() const { return m_textureResource.Get(); }	// テクスチャリソース取得
	size_t GetWidth() const { return width; }			// 幅取得
	size_t GetHeight() const { return height; }		// 高さ取得
	size_t GetMipLevels() const { return mipLevels; }	// ミップレベル数取得
	DXGI_FORMAT GetFormat() const { return format; }	// フォーマット取得

private:

	/// <summary>
	/// ファイルパスからスクラッチイメージとメタデータを取得する
	/// </summary>
	/// <param name="a_metaData">出力先メタデータ</param>
	/// <param name="a_img">出力先スクラッチイメージ</param>
	/// <param name="a_path">ファイルパス</param>
	/// <returns>成功 = true</returns>
	bool LoadFromPath(DirectX::TexMetadata& a_metaData,DirectX::ScratchImage& a_img,std::wstring& a_path);

	/// <summary>
	/// メタデータとスクラッチイメージからテクスチャクラスを構築する
	/// </summary>
	/// <param name="a_metaData">メタデータ</param>
	/// <param name="a_img">イメージデータ</param>
	/// <returns>成功 = true</returns>
	bool BuildFromScratchiImage(DirectX::TexMetadata& a_meta, DirectX::ScratchImage& a_sImg);

	/// <summary>
	/// アップロード用の中間ヒープの作成
	/// </summary>
	/// <param name="a_texDesc">作成したテクスチャの仕様書</param>
	/// <returns>作成した中間ヒープが帰る</returns>
	ID3D12Resource* CreateUploadHeap(const D3D12_RESOURCE_DESC& a_texDesc);

	/// <summary>
	/// GPUにテクスチャをコピーする
	/// </summary>
	/// <param name="a_dstLoca">コピー元</param>
	/// <param name="a_srcLoca">コピー先</param>
	void CopyTexRegion(D3D12_TEXTURE_COPY_LOCATION& a_dstLoca, D3D12_TEXTURE_COPY_LOCATION& a_srcLoca);

private:


	ComPtr<ID3D12Resource> m_textureResource;			// テクスチャリソース

	// メタデータ
	size_t width = 0;
	size_t height = 0;

	size_t mipLevels = 0;

	DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;
};