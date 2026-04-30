#pragma once

namespace Engine::Resource
{
	// モデル読み込み
	Engine::Resource::ModelData ImportModel(const std::string& a_filePath);

	class ModelLoader
	{
	public:

		Handle<Model> Load(const Engine::GUID& a_guid);
		Handle<Model> Request(const std::string& a_path);

	private:

		std::unordered_map<Engine::GUID, Handle<Model>> m_cache;

	};
}