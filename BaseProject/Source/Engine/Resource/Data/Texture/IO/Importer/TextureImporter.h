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
		// ComPtr で保持すること。
		// 生ポインタにすると CreateCommittedResource が付けた参照カウント1が
		// 誰にも解放されず、アップロードバッファが永久にリークする
		// (終了時の LIVE_RESOURCE として大量に残る原因だった)。
		ComPtr<ID3D12Resource> pResource = nullptr;

		UINT subresourceCount = 0;

		std::vector<D3D12_PLACED_SUBRESOURCE_FOOTPRINT> layoutVec;
		std::vector<UINT> numRowVec;
		std::vector<UINT64> rowSizeVec;
	};

	//----------------------------------------------------------------------------------------------
	// どれもGPUへの転送を伴うので、ビルドコンテキストを受け取る。
	// デバイスと転送の依頼先(グラフィックスエンジン)はコンテキストから引く
	//----------------------------------------------------------------------------------------------

	// テクスチャ読み込み
	ComPtr<ID3D12Resource> ImportTexture(
		const ResourceBuildContext& a_ctx,
		const std::string& a_filePath,
		D3D12_RESOURCE_DESC* a_desc = nullptr
	);

	// 色を指定してデフォルトテクスチャ生成
	ComPtr<ID3D12Resource> DefaultTexture(const ResourceBuildContext& a_ctx, Math::Color a_color);

	// 白テクスチャ
	ComPtr<ID3D12Resource> WhiteTexture(const ResourceBuildContext& a_ctx);

	// 黒テクスチャ
	ComPtr<ID3D12Resource> BlackTexture(const ResourceBuildContext& a_ctx);

	// ノーマルマップ白テクスチャ
	ComPtr<ID3D12Resource> NormalWhiteTexture(const ResourceBuildContext& a_ctx);

	// ORMテクスチャ
	ComPtr<ID3D12Resource> ORMTexture(const ResourceBuildContext& a_ctx);
}
