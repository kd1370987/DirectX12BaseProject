#include "ModelLoader.h"
#include "../../Manager/AssetDatabase/AssetDatabase.h"
#include "../../Manager/ResourceManager/ResourceManager.h"
namespace Engine::Resource
{
	std::unordered_map<Engine::GUID, Engine::Handle<Engine::Resource::Model>>
		Engine::Resource::ModelLoader::m_cache;

	Handle<Model> ModelLoader::Load(const Engine::GUID& a_guid)
	{
		// すでに読み込み済みならそのハンドルを返す
		auto _it = m_cache.find(a_guid);
		if (_it != m_cache.end())
		{
			return _it->second;
		}

		// なければロード
		auto _path = Resource::AssetDatabase::Instance().GetFilePathFromGUID(a_guid);
		if (_path.empty()) return Handle<Model>();

		Model _model = {};
		_model.Import(_path);

		//リソースマネージャーに登録
		auto _handle = ResourceManager::Instance().Add(std::move(_model));

		// キャッシュに登録
		m_cache.emplace(a_guid, _handle);

		return _handle;
	}
	Handle<Model> ModelLoader::Request(const std::string& a_path)
	{
		// アセットデータベースに問い合わせ
		auto _guid = Resource::AssetDatabase::Instance().GetGUIDFromFilePath(a_path);
		if (_guid != Engine::DefaultGUID)
		{
			return Load(_guid);
		}

		// GUIDがなければ
		// フォルダ以下をもう一度探す処理を入れる予定だが、とりあえず、エラー値を返す
		return Handle<Model>();
	}
	const std::unordered_map<Engine::GUID, Handle<Model>>& ModelLoader::GetAllCache()
	{
		return m_cache;
	}
	const Handle<Model>& ModelLoader::GetHandle(const Engine::GUID& a_guid)
	{
		// すでに読み込み済みならそのハンドルを返す
		auto _it = m_cache.find(a_guid);
		if (_it != m_cache.end())
		{
			return _it->second;
		}
		// なければエラーハンドルを返す
		return Handle<Model>();
	}
	std::string ModelLoader::GetFilePath(Handle<Model> a_handle)
	{
		auto _guid = GetGUID(a_handle);
		auto _path = AssetDatabase::Instance().GetFilePathFromGUID(_guid);
		return _path;
	}
	const Engine::GUID& ModelLoader::GetGUID(const Handle<Model>& a_handle)
	{
		for (auto& [_guid,_handle] : m_cache)
		{
			if (_handle == a_handle)
			{
				return _guid;
			}
		}

		return Engine::DefaultGUID;
	}
	bool ModelLoader::Has(const Engine::GUID& a_guid)
	{
		auto _it = m_cache.find(a_guid);
		if (_it != m_cache.end())
		{
			return true;
		}
		return false;
	}
}