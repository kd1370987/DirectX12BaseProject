#include "AnimatorAssetIO.h"
#include "../../../Data/AnimatorAsset/AnimatorAsset.h"

#include "../../../Manager/AssetDatabase/AssetDatabase.h"
#include "../../../Manager/ResourceManager/ResourceManager.h"
namespace Engine::Resource
{
	AnimatorAsset AnimatorAssetIO::LoadFromFile(const std::string& a_path)
	{
		AnimatorAsset _asset = {};
		_asset.Load(a_path);
		return _asset;
	}
	void AnimatorAssetIO::Create(
		const std::string& a_path,
		const std::string& a_name
	)
	{
		// ディレクトリ(既存資産の場所はそのまま)
		static std::string _dir = "Asset/StateMachine/";
		auto _basePath = _dir + a_path + "/" + a_name;

		// すでにないかチェック
		Engine::GUID _checkGUID = AssetDatabase::Instance().GetGUIDFromFilePath(_basePath);
		if (_checkGUID != Engine::DefaultGUID)
		{
			ENGINE_LOG("すでに作成されたアニメーターです : %s", _basePath.c_str());
			return;
		}

		// アセットデータベースに場所を作る
		auto _guid = AssetDatabase::Instance().AddMetaData(_basePath, "AnimatorAsset");

		// リソースマネージャーに登録
		AnimatorAsset _asset = {};
		_asset.SetName(a_name);
		_asset.Save(_basePath);

		// 新規登録
		ResourceManager::Instance().AddResourceAndGUID(std::move(_asset), _guid);

		return;
	}
}
