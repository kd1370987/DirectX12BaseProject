#pragma once

namespace Engine::Resource
{
	class TextureLoader
	{
	public:

		// テクスチャの読み込み
		static Handle<Texture> Load(const Engine::GUID& a_guid);
		static Handle<Texture> Request(const std::string& a_path);

		// テクスチャの作成
		static Handle<Texture> Create(const TextureCreateDesc& a_initData);

		// キャッシュの取得
		static const std::unordered_map<Engine::GUID, Handle<Texture>>& GetAllCache();
		static const std::unordered_map<std::string, Handle<Texture>>& GetAllNameCache();

	private:

		// デフォルトテクスチャの要求
		static Handle<Texture> RequestDefaultTex();

		// ビューの作成
		static void CreateView(const Handle<Texture>& a_handle);

	private:

		static std::unordered_map<Engine::GUID, Handle<Texture>> m_cache;
		static std::unordered_map<std::string, Handle<Texture>> m_nameCache;
	};
}