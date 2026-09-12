#include "AnimatorAssetIO.h"
#include "../../../Data/AnimatorAsset/AnimatorAsset.h"

#include "../../../Manager/AssetDatabase/AssetDatabase.h"
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

		// 書き出すだけでよい。
		// メタファイルとGUIDは、AssetDatabase の監視が新しいファイルを見つけて用意する
		AnimatorAsset _asset = {};
		_asset.SetName(a_name);
		_asset.Save(_basePath);
	}
}
