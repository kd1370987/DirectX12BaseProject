#pragma once

namespace Engine::Resource
{
	class TextureLoader
	{
	public:

		// テクスチャの読み込み
		static Handle<Texture> Load(const Engine::GUID& a_guid, const DXSM::Color& a_data = {255,255,255,255});

		// ファイルパスからの読み込み。なければデフォルトテクスチャを返す
		static Handle<Texture> Request(const std::string& a_path, const DXSM::Color& a_data);

		// テクスチャの作成
		static Handle<Texture> Create(const TextureCreateDesc& a_initData);

		

		// キャッシュの取得
		static const std::unordered_map<Engine::GUID, Handle<Texture>>& GetAllCache();
		static const std::unordered_map<std::string, Handle<Texture>>& GetAllNameCache();

	private:

		// デフォルトテクスチャの要求
		static Handle<Texture> RequestDefaultTex(const std::string& a_name,const DXSM::Color& a_data);

	private:

		static std::unordered_map<Engine::GUID, Handle<Texture>> m_cache;
		static std::unordered_map<std::string, Handle<Texture>> m_nameCache;
	};
}