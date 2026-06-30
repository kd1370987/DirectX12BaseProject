#include "ParticlesLoader.h"

#include "../../Data/Particles/ParticlesAsset.h"

#include "../../Manager/AssetDatabase/AssetDatabase.h"
#include "../../Manager/ResourceManager/ResourceManager.h"

namespace Engine::Resource
{
	ParticlesAsset ParticlesAssetLoader::LoadFromFile(const std::string& a_path)
	{
		ParticlesAsset _pa = {};
		_pa.Load(a_path);
		return _pa;
	}
	void ParticlesAssetLoader::Create(const std::string& a_path, const std::string& a_name)
	{
		// ディレクトリ
		static std::string _dir = "Asset/ParticlesAsset/";
		auto _basePath = _dir + a_path +"/" + a_name;

		// すでにないかチェック
		Engine::GUID _checkGUID = AssetDatabase::Instance().GetGUIDFromFilePath(_basePath);
		if (_checkGUID != Engine::DefaultGUID)
		{
			// すでに作成されていた場合
			ENGINE_LOG("すでに作成済みのパーティクルです : %s",_basePath.c_str());
			return;
		}


		// アセットデータベースに場所を作る
		auto _guid = AssetDatabase::Instance().AddMetaData(_basePath, "ParticlesAsset");

		// リソースマネージャーに登録
		ParticlesAsset _sma = {};
		_sma.Create(a_name, _guid);
		_sma.Save(_basePath);
		ResourceManager::Instance().AddResourceAndGUID(std::move(_sma),_guid);
	}
}