#pragma once

namespace Engine::Resource
{
	class TextureLoader
	{
	public:
		// ファイルパスからの読み込み。なければデフォルトテクスチャを返す
		//static Handle<Texture> Request(const std::string& a_path, const DXSM::Color& a_data);
		static Engine::GUID RequestGUID(const std::string& a_path, const DXSM::Color& a_data);		// なければデフォルト値を返す

		// テクスチャの作成
		static Handle<Texture> Create(const TextureCreateDesc& a_initData);

		

		// キャッシュの取得
		static const std::unordered_map<Engine::GUID, Handle<Texture>>& GetAllCache();
		static const std::unordered_map<std::string, Handle<Texture>>& GetAllNameCache();

		static bool Has(const Engine::GUID& a_guid);

		static Engine::GUID GetGUIDFromHandle(const Handle<Texture>& a_handle);

		/// <summary>
		/// GUIDからハンドルを取得
		/// </summary>
		static Handle<Texture> GetHandle(const Engine::GUID&  a_guid);

		/// <summary>
		/// テクスチャの読み込み。パスはある前提なので外部でチェック必須
		/// </summary>
		static Texture LoadFromFile(const std::string& a_path);

		/// <summary>
		/// 動的テクスチャ作成
		/// </summary>
		//static Texture Create(const TextureCreateDesc& a_initData);

		/// <summary>
		/// 単色テクスチャ作成 
		/// </summary>
		/// <param name="a_color">指定色</param>
		/// <returns>テクスチャの実態</returns>
		static Texture CreateColorTexture(const DXSM::Color& a_color);

		/// <summary>
		/// 色からGUIDを返す
		/// </summary>
		static Engine::GUID GetColorGUID(const DXSM::Color& a_color);

		/// <summary>
		/// マテリアルから呼ばれるロード関数
		/// </summary>
		static Handle<Texture> LoadTexture(const Engine::GUID& a_guid,const DXSM::Color& a_defaultColor);

	private:

		// デフォルトテクスチャの要求
		static Handle<Texture> RequestDefaultTex(const std::string& a_name,const DXSM::Color& a_data);

	private:

		static std::unordered_map<Engine::GUID, Handle<Texture>> m_cache;
		static std::unordered_map<std::string, Handle<Texture>> m_nameCache;
	};
}