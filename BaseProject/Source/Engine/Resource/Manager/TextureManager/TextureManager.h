#pragma once

#include "../../Data/Texture/Texture.h";

namespace Engine::Resource
{
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
		Engine::Resource::Handle<Engine::Resource::TextureRes> LoadTexture(const std::string& a_path);
		Engine::Resource::HandleRange<Engine::Resource::TextureRes> LoadTextureRange(
			const std::vector<std::string>& a_pathVec
		);

		//------------------------------------------------------------------------------------------
		// リソース作成
		//------------------------------------------------------------------------------------------
		Engine::Resource::Handle<Engine::Resource::TextureRes> CreateTexture(
			const std::string& a_name,
			const UINT64& a_width,
			const UINT& a_height,
			const DXGI_FORMAT&  a_format,
			const TextureUsage& a_usage
		);

		//------------------------------------------------------------------------------------------
		// リソース取得
		//------------------------------------------------------------------------------------------
	private:

		//------------------------------------------------------------------------------------------
		// リソースの管理
		//------------------------------------------------------------------------------------------
		Engine::Resource::Handle<Engine::Resource::TextureRes> Add(const TextureRes& a_Texture);		// 追加
		void Subtract(const Engine::Resource::Handle<Engine::Resource::TextureRes>& a_handle);		// 削除

	private:

		// テクスチャデータ管理
		std::unordered_map<std::string, Handle<TextureRes>> m_handleMap = {};		// 重なり防止
		std::vector<SharedSlot<TextureRes>> m_slotStorage = {};					// 実データ

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