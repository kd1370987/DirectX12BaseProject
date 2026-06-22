#include "ParticlesLoader.h"

#include "../../Data/Particles/ParticlesAsset.h"

#include "../../Manager/AssetDatabase/AssetDatabase.h"
#include "../../Manager/ResourceManager/ResourceManager.h"

namespace Engine::Resource
{
	Handle<ParticlesAsset> Engine::Resource::ParticlesAssetLoader::Load(const Engine::GUID& a_guid)
	{
		// 読み込みチェック
		if (Has(a_guid))
		{
			return m_cache[a_guid];
		}

		// なければロード
		auto _path = AssetDatabase::Instance().GetFilePathFromGUID(a_guid);
		auto _fileName = AssetDatabase::Instance().GetFileNameFromGUID(a_guid);

		if (_path.empty()) return Handle<ParticlesAsset>();

		ParticlesAsset _sma = {};
		auto _dir = FileUtility::GetDirFromPath(_path);
		_sma.Load(_dir,_fileName);

		// リソースマネージャーに登録
		auto _handle = ResourceManager::Instance().Add(std::move(_sma));
		m_cache[a_guid] = _handle;
		return _handle;
	}
	std::pair<Engine::GUID, Handle<ParticlesAsset>> ParticlesAssetLoader::Create(const std::string& a_path, const std::string& a_name)
	{
		// ディレクトリ
		static std::string _dir = "Asset/ParticlesAsset/";
		auto _basePath = _dir + a_path + a_name;

		// すでにないかチェック
		Engine::GUID _checkGUID = AssetDatabase::Instance().GetGUIDFromFilePath(_basePath);
		if (_checkGUID != Engine::DefaultGUID)
		{
			// すでに作成されていた場合
			auto _handle = Load(_checkGUID);
			std::pair<Engine::GUID, Handle<ParticlesAsset>> _res;
			_res.first = _checkGUID;
			_res.second = _handle;
			return _res;
		}


		// アセットデータベースに場所を作る
		auto _guid = AssetDatabase::Instance().AddMetaData(_basePath, "ParticlesAsset");

		// リソースマネージャーに登録
		ParticlesAsset _sma = {};
		_sma.Create(a_name, _guid);
		_sma.Save(_basePath);
		auto _handle = ResourceManager::Instance().Add(std::move(_sma));
		m_cache[_guid] = _handle;

		// 返す
		std::pair<Engine::GUID, Handle<ParticlesAsset>> _res;
		_res.first = _guid;
		_res.second = _handle;
		return _res;
	}
}