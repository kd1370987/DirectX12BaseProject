#include "AnimatorAssetIO.h"
#include "../../../Data/AnimatorAsset/AnimatorAsset.h"

#include "../../../Manager/AssetDatabase/AssetDatabase.h"
#include "../../../Manager/ResourceManager/ResourceManager.h"
namespace Engine::Resource
{
	AnimatorAsset AnimatorAssetIO::LoadFromFile(const std::string& a_path, ResourceManager& a_resourceManager)
	{
		AnimatorAsset _asset = {};
		_asset.Load(a_path, a_resourceManager);
		return _asset;
	}
	void AnimatorAssetIO::Create(ResourceManager& a_resourceManager, const std::string& a_path, const std::string& a_name)
	{
		// ディレクトリ(既存資産の場所はそのまま)
		static std::string _dir = "Asset/StateMachine/";
		auto _basePath = _dir + a_path + "/" + a_name;

		// すでにないかチェック
		Engine::GUID _checkGUID = a_resourceManager.RefAssetDatabase().GetGUIDFromFilePath(_basePath);
		if (_checkGUID != Engine::DefaultGUID)
		{
			ENGINE_LOG("すでに作成されたアニメーターです : %s", _basePath.c_str());
			return;
		}

		// 書き出すだけでよい。
		// メタファイルとGUIDは、AssetDatabase の監視が新しいファイルを見つけて用意する
		AnimatorAsset _asset = {};
		_asset.SetName(a_name);
		_asset.Save(_basePath, a_resourceManager);
	}
}
