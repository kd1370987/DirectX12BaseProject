#pragma once
namespace Engine::Resource
{
	class ModelLoader
	{
	public:

		static Handle<Model> Load(const Engine::GUID& a_guid);
		static Handle<Model> Request(const std::string& a_path);

		static const std::unordered_map<Engine::GUID, Handle<Model>>& GetAllCache();

	private:

		static std::unordered_map<Engine::GUID, Handle<Model>> m_cache;
	};
}