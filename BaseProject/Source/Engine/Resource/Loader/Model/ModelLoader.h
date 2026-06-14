#pragma once
namespace Engine::Resource
{
	class ModelLoader
	{
	public:

		static Handle<Model> Load(const Engine::GUID& a_guid);
		static Handle<Model> Request(const std::string& a_path);

		static const std::unordered_map<Engine::GUID, Handle<Model>>& GetAllCache();
		static const Handle<Model>& GetHandle(const Engine::GUID& a_guid);
		static std::string GetFilePath(Handle<Model> a_handle);
		static const Engine::GUID& GetGUID(const Handle<Model>& a_handle);

		static bool Has(const Engine::GUID& a_guid);

	private:

		static std::unordered_map<Engine::GUID, Handle<Model>> m_cache;
	};
}