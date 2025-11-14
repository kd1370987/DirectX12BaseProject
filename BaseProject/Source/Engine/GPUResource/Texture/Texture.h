#pragma once

class Texture
{
public:

	Texture() = default;
	~Texture() = default;
	//=================================================
	// アクセサ
	//=================================================
	ID3D12Resource* GetResource() const { return m_textureResource.Get(); }	// テクスチャリソース取得
	D3D12_GPU_DESCRIPTOR_HANDLE GetGpuSrvHandle() const { return m_gpuSrvHandle; }	// GPU側SRVハンドル取得
	D3D12_CPU_DESCRIPTOR_HANDLE GetCpuSrvHandle() const { return m_cpuSrvHandle; }	// CPU側SRVハンドル取得
	size_t GetWidth() const { return width; }			// 幅取得
	size_t GetHeight() const { return height; }		// 高さ取得
	size_t GetMipLevels() const { return mipLevels; }	// ミップレベル数取得
	DXGI_FORMAT GetFormat() const { return format; }	// フォーマット取得

	bool Load(const std::string& a_path);


private:

	ComPtr<ID3D12Resource> m_textureResource;			// テクスチャリソース
	D3D12_GPU_DESCRIPTOR_HANDLE m_gpuSrvHandle;			// SRVハンドル
	D3D12_CPU_DESCRIPTOR_HANDLE m_cpuSrvHandle;			// CPU側SRVハンドル

	// メタデータ
	size_t width = 0;
	size_t height = 0;

	size_t mipLevels = 0;

	DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;
};