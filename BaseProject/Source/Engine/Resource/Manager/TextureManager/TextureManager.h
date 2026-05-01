#pragma once

namespace Engine::Resource
{
	struct TextureInit
	{
		std::string pathName = "";
		DirectX::XMFLOAT4 data = {255,255,255,255};
	};

	//==========================================================================================
	// 
	// テクスチャ管理クラス
	// 
	//==========================================================================================
	class TextureManager
	{
	public:
		//------------------------------------------------------------------------------------------
		// リソースの読み込み
		//------------------------------------------------------------------------------------------
		Handle<Texture> LoadTexture(const std::string& a_path, const DirectX::XMFLOAT4& a_data = { 255,255,255,255 });
		

		//------------------------------------------------------------------------------------------
		// リソース作成
		//------------------------------------------------------------------------------------------
		Handle<Texture> CreateTexture(const TextureCreateDesc& a_init);

		//------------------------------------------------------------------------------------------
		// リソース取得
		//------------------------------------------------------------------------------------------
		const Texture& GetTexture(const Handle<Texture>& a_handle);
		const Texture& GetTexture(const std::string& a_name);
		Texture& RefTexture(const Handle<Texture>& a_handle);
		Texture& RefTexture(const std::string& a_name);
		
		std::unordered_map<std::string, Handle<Texture>>& RefAllTex();
		std::vector<Texture>& GetAllTex();

		const Handle<Texture>& GetHandle(const std::string& a_name);

	private:

		//------------------------------------------------------------------------------------------
		// リソースの管理
		//------------------------------------------------------------------------------------------
		Handle<Texture> Add(const Texture& a_Texture);		// 追加
		void Subtract(const Handle<Texture>& a_handle);		// 削除

		// ビュー作成
		void CreateView(const Handle<Texture>& a_outTex);

		// 登録されているかのチェック
		bool Has(const std::string& a_name) const;

	private:

		// 重なり防止
		std::unordered_map<std::string, Handle<Texture>> m_nameMap = {};

		// ハンドルで実データ管理
		Storage::HandleStorage<Texture> m_handleStorage;		// ハンドルストレージ
		std::vector<Texture>			m_texData = {};			// テクスチャデータ

	private:
		TextureManager();
		~TextureManager();
	public:
		static TextureManager& Instance()
		{
			static TextureManager _instance;
			return _instance;
		}
	};
}