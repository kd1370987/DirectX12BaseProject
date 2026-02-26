#pragma once

struct Texture;

struct UploadBuffer
{
	ID3D12Resource* pResource = nullptr;

	UINT subresourceCount = 0;

	std::vector<D3D12_PLACED_SUBRESOURCE_FOOTPRINT> layoutVec;
	std::vector<UINT> numRowVec;
	std::vector<UINT64> rowSizeVec;
};

namespace TextureLoad
{
	/// <summary>
	/// 初期化してテクスチャを読み込む
	/// </summary>
	/// <param name="a_path">ファイルパス</param>
	/// <param name="a_dstTex">出力変数</param>
	/// <param name="a_desc">生成時の設定</param>
	/// <returns>読み込みに成功 = true</returns>
	bool Load(const std::string& a_path, Texture& a_dstTex, D3D12_RESOURCE_DESC* a_desc = nullptr);

	Texture Default(DirectX::XMFLOAT4 a_color);

	// 白テクスチャ
	Texture White();

	// 黒テクスチャ
	Texture Black();

	// ノーマルマップ白テクスチャ
	Texture NormalWhite();

	// ORMテクスチャ
	Texture ORM();
}