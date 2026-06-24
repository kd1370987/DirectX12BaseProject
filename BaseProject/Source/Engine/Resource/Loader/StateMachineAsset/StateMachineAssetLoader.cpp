#include "StateMachineAssetLoader.h"
#include "../../Data/StateMachineAsset/StateMachineAsset.h"

#include "../../Manager/AssetDatabase/AssetDatabase.h"
#include "../../Manager/ResourceManager/ResourceManager.h"
namespace Engine::Resource
{
	StateMachineAsset StateMachineAssetLoader::LoadFromFile(const std::string& a_path)
	{
		StateMachineAsset _sma = {};
		_sma.Load(a_path);
		return _sma;
	}
	void StateMachineAssetLoader::Create(
		const std::string& a_path,
		const std::string& a_name
	)
	{
		// ディレクトリ
		static std::string _dir = "Asset/StateMachin/";
		auto _basePath = _dir + a_path + a_name;

		// すでにないかチェック
		Engine::GUID _checkGUID = AssetDatabase::Instance().GetGUIDFromFilePath(_basePath);
		if (_checkGUID != Engine::DefaultGUID)
		{
			ENGINE_LOG("すでに作成されたステートマシンです : %s",_basePath.c_str());
			return;
		}

		// アセットデータベースに場所を作る
		auto _guid = AssetDatabase::Instance().AddMetaData(_basePath,"StateMachinAsset");

		// リソースマネージャーに登録
		StateMachineAsset _sma = {};
		_sma.SetName(a_name);
		_sma.Save(_basePath);

		// 新規登録
		ResourceManager::Instance().AddResourceAndGUID(std::move(_sma), _guid);

		return;
	}
}