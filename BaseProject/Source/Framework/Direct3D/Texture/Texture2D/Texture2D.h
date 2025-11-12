#pragma once

class DescriptorHeap;
class DescriptorHandle;

class Texture2D
{
public:

	static Texture2D* Get(std::string a_path);		// stringで受け取ったパスからテクスチャを読み込む
	static Texture2D* Get(std::wstring a_path);		// wstringで受け取ったパスからテクスチャを読み込む
	static Texture2D* GetWhite();					// 白テクスチャを取得	
	bool IsValid() const;							// テクスチャが正常に読み込まれているかどうか

	ID3D12Resource* Resource();						// テクスチャリソースを取得
	D3D12_SHADER_RESOURCE_VIEW_DESC ViewDesc();		// SRVの設定を返す

private:

	bool m_isValid = false;							// テクスチャが正常に読み込まれているかどうか
	Texture2D(std::string a_path);
	Texture2D(std::wstring a_path);
	Texture2D(ID3D12Resource* a_buffer);
	ComPtr<ID3D12Resource> m_pResource = nullptr;	// テクスチャリソース
	bool Load(std::string& a_path);
	bool Load(std::wstring& a_path);

	static ID3D12Resource* GetDefaultResource(size_t a_width, size_t a_height);

	// コピー禁止
	Texture2D(const Texture2D&) = delete;
	void operator=(const Texture2D&) = delete;

};