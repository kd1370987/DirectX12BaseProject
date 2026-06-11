#pragma once
namespace Engine::Resource
{
	class StateMachineAsset;

	class StateMachineAssetLoader
	{
	public:

		// 読み込み
		static Handle<StateMachineAsset> Load(const Engine::GUID& a_guid);
		
		// リクエスト
		static std::pair<Engine::GUID,Handle<StateMachineAsset>> Create(const std::string& a_path,const std::string& a_name);
		
		// アクセサ
		static const std::unordered_map<Engine::GUID, Handle<StateMachineAsset>>& GetAllCache();
		
	private:

		static std::unordered_map<Engine::GUID, Handle<StateMachineAsset>>			m_cache;
	};
}