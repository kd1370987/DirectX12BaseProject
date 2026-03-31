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
		Engine::Resource::Handle<Engine::Resource::Texture> LoadTexture(const std::string& a_path);
		std::vector<Engine::Resource::Handle<Engine::Resource::Texture>> LoadTextureRange(
			const std::vector<TextureInit>& a_initVec
		);

		//------------------------------------------------------------------------------------------
		// リソース作成
		//------------------------------------------------------------------------------------------
		Engine::Resource::Handle<Engine::Resource::Texture> CreateTexture(
			const std::string& a_name,
			const UINT64& a_width,
			const UINT& a_height,
			const DXGI_FORMAT&  a_format,
			const TextureUsage& a_usage
		);

		//------------------------------------------------------------------------------------------
		// リソース取得
		//------------------------------------------------------------------------------------------
		const Engine::Resource::Texture& GetTexture(
			const Engine::Resource::Handle<Engine::Resource::Texture>& a_handle
		);
		Engine::Resource::Texture& RefTexture(
			const Engine::Resource::Handle<Engine::Resource::Texture>& a_handle
		);
		
		std::unordered_map<std::string, Engine::Resource::Handle<Engine::Resource::Texture>>& RefAllTex();
		std::vector<Engine::Resource::Texture>& GetAllTex();

	private:

		//------------------------------------------------------------------------------------------
		// リソースの管理
		//------------------------------------------------------------------------------------------
		Engine::Resource::Handle<Engine::Resource::Texture> Add(const Texture& a_Texture);		// 追加
		void Subtract(const Engine::Resource::Handle<Engine::Resource::Texture>& a_handle);		// 削除

		// ビュー作成
		void CreateView(const std::vector<Engine::Resource::Handle<Engine::Resource::Texture>>& a_outTex);

		// 登録されているかのチェック
		bool Has(const std::string& a_name) const;

	private:

		// 重なり防止
		std::unordered_map<std::string, Handle<Texture>> m_nameMap = {};

		// ハンドルで実データ管理
		Storage::HandleStorage<Texture> m_handleStorage;		// ハンドルストレージ
		std::vector<Texture>			m_texData = {};			// テクスチャデータ
		std::vector<UINT>				m_sharedCount = {};		// 共有カウント

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