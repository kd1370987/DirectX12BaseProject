#pragma once

namespace Engine::Resource
{
	struct TextureInit
	{
		std::string pathName = "";
		DirectX::XMFLOAT4 data = {255,255,255,255};
	};

	struct CreateTextureDesc
	{
		std::string name = "CreateTexture";
		UINT64 width = 0;
		UINT height = 0;
		DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM;
		TextureUsage usage = TextureUsage::None;
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
		Engine::Resource::Handle<Engine::Resource::Texture> CreateTexture(const CreateTextureDesc& a_init);

		//------------------------------------------------------------------------------------------
		// リソース取得
		//------------------------------------------------------------------------------------------
		const Engine::Resource::Texture& GetTexture(
			const Engine::Resource::Handle<Engine::Resource::Texture>& a_handle
		);
		const Texture& GetTexture(const std::string& a_name);
		Engine::Resource::Texture& RefTexture(
			const Engine::Resource::Handle<Engine::Resource::Texture>& a_handle
		);
		Texture& RefTexture(const std::string& a_name);
		
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