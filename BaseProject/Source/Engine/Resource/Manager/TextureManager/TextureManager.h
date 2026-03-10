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
		std::vector<Engine::Resource::SharedSlot<Engine::Resource::Texture>>& GetAllTex();

	private:

		//------------------------------------------------------------------------------------------
		// リソースの管理
		//------------------------------------------------------------------------------------------
		Engine::Resource::Handle<Engine::Resource::Texture> Add(const Texture& a_Texture);		// 追加
		void Subtract(const Engine::Resource::Handle<Engine::Resource::Texture>& a_handle);		// 削除

		// ビュー作成
		void CreateView(Engine::Resource::Texture& a_outTex);
		
		void CreateView(const std::vector<Engine::Resource::Handle<Engine::Resource::Texture>>& a_outTex);

		// 世代チェック
		bool GenCheck(const Engine::Resource::Handle<Engine::Resource::Texture>& a_handle) const;

	private:

		// テクスチャデータ管理
		std::unordered_map<std::string, Handle<Texture>> m_handleMap = {};		// 重なり防止
		std::vector<SharedSlot<Texture>> m_slotStorage = {};					// 実データ

		// 使用可能場所リスト
		std::queue<Index> m_indexQueue = {};
		UINT m_indexQueueMaxSize = 0;

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