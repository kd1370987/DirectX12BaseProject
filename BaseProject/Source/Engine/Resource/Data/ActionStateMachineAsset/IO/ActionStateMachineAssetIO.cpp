#include "ActionStateMachineAssetIO.h"
#include "../../../Data/ActionStateMachineAsset/ActionStateMachineAsset.h"

#include "../../../Manager/AssetDatabase/AssetDatabase.h"
#include "../../../Manager/ResourceManager/ResourceManager.h"
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

		auto _guid = AssetDatabase::Instance().AddMetaData(_basePath, "ActionStateMachineAsset");

		ActionStateMachineAsset _asset = {};
		_asset.SetName(a_name);
		_asset.Save(_basePath);

		ResourceManager::Instance().AddResourceAndGUID(std::move(_asset), _guid);

		return;
	}
}
