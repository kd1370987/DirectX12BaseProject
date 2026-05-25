//#include "MaterialLoader.h"
//#include "../../Manager/AssetDatabase/AssetDatabase.h"
//#include "../../Manager/ResourceManager/ResourceManager.h"
//namespace Engine::Resource
//{
//	std::unordered_map<Engine::GUID, Handle<Material>> m_cache;
//	Handle<Material> Engine::Resource::MaterialLoader::Load(const Engine::GUID& a_guid)
//	{
//		// 読み込みチェック
//		auto _it = m_cache.find(a_guid);
//		if (_it != m_cache.end())
//		{
//			return _it->second;
//		}
//
//		// なければロード
//		auto _path = Resource::AssetDatabase::Instance().GetFilePathFromGUID(a_guid);
//		auto _dir = FileUtility::GetDirFromPath(_path);
//		Material _material = {};
//		_material.Load(_dir);
//
//		// リソースマネージャーに登録
//		auto _handle = Resource::ResourceManager::Instance().Add(std::move(_material));
//
//		// キャッシュに登録
//		m_cache.emplace(a_guid,_handle);
//
//		return _handle;
//	}
//}