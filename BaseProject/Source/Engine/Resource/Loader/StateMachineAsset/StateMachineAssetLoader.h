#pragma once

#include "../BaseLoader.h";

namespace Engine::Resource
{
	class StateMachineAsset;

	class StateMachineAssetLoader : public BaseLoader<StateMachineAsset,StateMachineAssetLoader>
	{
	public:

		// 読み込み
		static Handle<StateMachineAsset> Load(const Engine::GUID& a_guid);
		
		// リクエスト
		static std::pair<Engine::GUID,Handle<StateMachineAsset>> Create(const std::string& a_path,const std::string& a_name);

	};
}