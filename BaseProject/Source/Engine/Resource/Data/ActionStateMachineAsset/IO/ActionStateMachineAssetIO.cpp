#include "ActionStateMachineAssetIO.h"
#include "../../../Data/ActionStateMachineAsset/ActionStateMachineAsset.h"

#include "../../../Manager/AssetDatabase/AssetDatabase.h"
namespace Engine::Resource
{
	ActionStateMachineAsset ActionStateMachineAssetIO::LoadFromFile(const std::string& a_path)
	{
		ActionStateMachineAsset _asset = {};
		_asset.Load(a_path);
		return _asset;
	}

	void ActionStateMachineAssetIO::Create(
		const std::string& a_path,
		const std::string& a_name
	)
	{
		static std::string _dir = "Asset/ActionStateMachine/";
		auto _basePath = _dir + a_path + "/" + a_name;

		Engine::GUID _checkGUID = AssetDatabase::Instance().GetGUIDFromFilePath(_basePath);
		if (_checkGUID != Engine::DefaultGUID)
		{
			ENGINE_LOG("すでに作成されたステートマシンです : %s", _basePath.c_str());
			return;
		}

		// 書き出すだけでよい。
		// メタファイルとGUIDは、AssetDatabase の監視が新しいファイルを見つけて用意する
		ActionStateMachineAsset _asset = {};
		_asset.SetName(a_name);
		_asset.Save(_basePath);
	}
}
