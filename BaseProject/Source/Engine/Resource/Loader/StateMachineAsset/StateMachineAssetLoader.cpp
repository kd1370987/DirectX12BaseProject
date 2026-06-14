#include "StateMachineAssetLoader.h"
#include "../../Data/StateMachineAsset/StateMachineAsset.h"

#include "../../Manager/AssetDatabase/AssetDatabase.h"
#include "../../Manager/ResourceManager/ResourceManager.h"
namespace Engine::Resource
{
	std::unordered_map<Engine::GUID, Engine::Resource::Handle<Engine::Resource::StateMachineAsset>> Engine::Resource::StateMachineAssetLoader::m_cache;

	Handle<StateMachineAsset> StateMachineAssetLoader::Load(const Engine::GUID& a_guid)
	{
		// 読み込みチェック
		auto _it = m_cache.find(a_guid);
		if (_it != m_cache.end())
		{
			return _it->second;
		}

		// なければロード
		auto _path = AssetDatabase::Instance().GetFilePathFromGUID(a_guid);
		auto _fileName = AssetDatabase::Instance().GetFileNameFromGUID(a_guid);

		if (_path.empty()) return Handle<StateMachineAsset>();

		StateMachineAsset _sma = {};
		auto _dir = FileUtility::GetDirFromPath(_path);
		_sma.Load(_dir,_fileName);
		_sma.SetGUID(a_guid);

		// リソースマネージャーに登録
		auto _handle = ResourceManager::Instance().Add(std::move(_sma));
		m_cache[a_guid] = _handle;
		return _handle;
	}
	std::pair<Engine::GUID, Handle<StateMachineAsset>> StateMachineAssetLoader::Create(const std::string& a_path, const std::string& a_name)
	{
		// ディレクトリ
		static std::string _dir = "Asset/StateMachin/";
		auto _basePath = _dir + a_path + a_name;

		// すでにないかチェック
		Engine::GUID _checkGUID = AssetDatabase::Instance().GetGUIDFromFilePath(_basePath);
		if (_checkGUID != Engine::DefaultGUID)
		{
			// すでに作成されていた場合
			auto _handle = Load(_checkGUID);
			std::pair<Engine::GUID, Handle<StateMachineAsset>> _res;
			_res.first = _checkGUID;
			_res.second = _handle;
			return _res;
		}


		// アセットデータベースに場所を作る
		auto _guid = AssetDatabase::Instance().AddMetaData(_basePath,"StateMachinAsset");

		// リソースマネージャーに登録
		StateMachineAsset _sma = {};
		_sma.SetGUID(_guid);
		_sma.SetName(a_name);
		_sma.Save(_basePath);
		auto _handle = ResourceManager::Instance().Add(std::move(_sma));
		m_cache[_guid] = _handle;

		// 返す
		std::pair<Engine::GUID, Handle<StateMachineAsset>> _res;
		_res.first = _guid;
		_res.second = _handle;
		return _res;
	}
	const std::unordered_map<Engine::GUID, Handle<StateMachineAsset>>& StateMachineAssetLoader::GetAllCache()
	{
		return m_cache;
	}
	bool StateMachineAssetLoader::Has(const Engine::GUID& a_guid)
	{
		auto _it = m_cache.find(a_guid);
		if (_it != m_cache.end())
		{
			return true;
		}
		return false;
	}
}