#pragma once

namespace Engine::Graphics
{
	// レンダーグラフの抽象化されたリソース
	struct RGResource
	{
		std::string name = "None";
		uint32_t varsion = 0;
	};


	// レンダーグラフで使うリソースの抽象化IDを管理する
	class RGResourceManager
	{
	public:

		// 実態を登録
		void Register(
			const std::string& a_name,
			const DXGI_FORMAT& format,
			const UINT64& a_widht,
			const UINT& a_height,
			const Resource::TextureUsage& a_texUsage
		);

		// 既存の最新バージョンを取得する
		// 読み込み用 : テクスチャ名、読み込み時の使用方法
		RGResource Read(const std::string& a_resourceName, const Resource::TextureUsage& a_texUsage);

		// バージョンをインクリメントして新しいハンドルとして返す
		// 書き込み用 : テクスチャ名、書き込み時の使用方法
		RGResource Write(const std::string& a_resourceName, const Resource::TextureUsage& a_texUsage);

	private:

		struct ResourceData
		{
			// リソーステクスチャの作成情報
			std::string name = "none";
			DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;
			uint32_t widht = 0;
			uint32_t height = 0;
			Resource::TextureUsage usage = Resource::TextureUsage::None;

			// 実行順を決定するためのバージョン
			uint32_t currentVarsion = 0;

			// コンパイル時に作成される
			Resource::Handle<Resource::Texture> texHandle = {};
		};

		// リソース（テクスチャ）名、リソース情報
		std::unordered_map<std::string, ResourceData> m_resourceMap = {};

	};
};