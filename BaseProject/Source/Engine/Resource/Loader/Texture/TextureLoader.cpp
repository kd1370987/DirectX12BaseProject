#include "TextureLoader.h"

#include "../../Manager/AssetDatabase/AssetDatabase.h"
#include "../../Manager/ResourceManager/ResourceManager.h"

#include "Engine/D3D12/DescriptorHeapManager/DescriptorHeapManager.h"
#include "Engine/D3D12/D3D12Wrapper/D3D12Wrapper.h"

namespace Engine::Resource
{
	std::unordered_map<Engine::GUID, Engine::Handle<Engine::Resource::Texture>>
	Engine::Resource::TextureLoader::m_cache;
	std::unordered_map<std::string, Engine::Handle<Engine::Resource::Texture>>
	Engine::Resource::TextureLoader::m_nameCache;
	

	Handle<Texture> Engine::Resource::TextureLoader::Load(const Engine::GUID& a_guid, const DXSM::Color& a_data)
	{
		// すでに読み込み済みならそのハンドルを返す
		auto _it = m_cache.find(a_guid);
		if (_it != m_cache.end())
		{
			return _it->second;
		}

		// なければパスを検索してロードする
		auto _path = Resource::AssetDatabase::Instance().GetFilePathFromGUID(a_guid);
		if (_path.empty())
		{
			// パスが見つからなければデフォルトテクスチャを返す
			return RequestDefaultTex("Default",a_data);
		}

		// パスが見つかればテクスチャを読み込む
		Texture _Texture = {};
		_Texture.Import(_path);

		//リソースマネージャーに登録
		auto _handle = ResourceManager::Instance().Add(std::move(_Texture));

		// キャッシュに登録
		m_cache.emplace(a_guid, _handle);

		return _handle;
	}
	Handle<Texture> TextureLoader::Request(const std::string& a_path, const DXSM::Color& a_data)
	{
		// アセットデータベースに問い合わせ
		auto _guid = Resource::AssetDatabase::Instance().GetGUIDFromFilePath(a_path);
		if (_guid != Engine::DefaultGUID)
		{
			return Load(_guid,a_data);
		}

		return RequestDefaultTex("Default",a_data);

		// GUIDがなければ
		// フォルダ以下をもう一度探す処理を入れる予定だが、とりあえず、エラー値を返す
		assert(0 && "検索したテクスチャがありません");
		return Handle<Texture>();
	}
	Engine::GUID TextureLoader::RequestGUID(const std::string& a_path, const DXSM::Color& a_data)
	{
		auto _guid = Resource::AssetDatabase::Instance().GetGUIDFromFilePath(a_path);
		return _guid;
	}
	Handle<Texture> TextureLoader::Create(const TextureCreateDesc& a_initData)
	{
		// 作成済みかのチェック
		auto _it = m_nameCache.find(a_initData.name);
		if (_it != m_nameCache.end())
		{
			return _it->second;
		}

		// テクスチャ作成
		Texture _tex;
		_tex.Create(a_initData);

		// リソースマネージャーに登録
		auto _handle = ResourceManager::Instance().Add(std::move(_tex));

		// ハンドルキャッシュを追加
		m_nameCache.emplace(a_initData.name,_handle);

		return _handle;
	}
	const std::unordered_map<Engine::GUID, Handle<Texture>>& TextureLoader::GetAllCache()
	{
		return m_cache;
	}
	const std::unordered_map<std::string, Handle<Texture>>& TextureLoader::GetAllNameCache()
	{
		return m_nameCache;
	}
	bool TextureLoader::Has(const Engine::GUID& a_guid)
	{
		auto _it = m_cache.find(a_guid);
		if (_it != m_cache.end())
		{
			return true;
		}
		return false;
	}
	Engine::GUID TextureLoader::GetGUIDFromHandle(const Handle<Texture>& a_handle)
	{
		for (auto& [_guid, _handle] : m_cache)
		{
			if (_handle == a_handle)
			{
				return _guid;
			}
		}
		return Engine::DefaultGUID;
	}
	Handle<Texture> TextureLoader::GetHandle(const Engine::GUID& a_guid)
	{
		auto _it = m_cache.find(a_guid);
		if (_it != m_cache.end())
		{
			return _it->second;
		}

		return Handle<Texture>();
	}
	Handle<Texture> TextureLoader::RequestDefaultTex(const std::string& a_name, const DXSM::Color &a_data)
	{
		std::string _checkSTR = "";
		// カラーチェック
		if (a_data == TexColor::WHITE)
		{
			_checkSTR = "WHITE";
		}
		if (a_data == TexColor::BLACK)
		{
			_checkSTR = "BLACK";
		}
		if (a_data == TexColor::NORMAL)
		{
			_checkSTR = "NORMAL";
		}
		if (a_data == TexColor::ORM)
		{
			_checkSTR = "ORM";

		}

		std::string _name = a_name + _checkSTR;

		// 作成済みかのチェック
		auto _it = m_nameCache.find(_name);
		if (_it != m_nameCache.end())
		{
			return _it->second;
		}

		// テクスチャ作成
		Texture _tex;
		_tex.Create(_name,a_data);

		// リソースマネージャーに登録
		auto _handle = ResourceManager::Instance().Add(std::move(_tex));

		// ハンドルキャッシュを追加
		m_nameCache.emplace(_name, _handle);

		return _handle;
	}
}