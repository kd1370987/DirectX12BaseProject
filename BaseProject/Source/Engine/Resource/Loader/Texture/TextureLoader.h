#pragma once

namespace Engine::Resource
{
	class TextureLoader
	{
	public:

		static Handle<Texture> Load(const Engine::GUID& a_guid);
		static Handle<Texture> Request(const std::string& a_path);

		static Handle<Texture> Create();

		static const std::unordered_map<Engine::GUID, Handle<Texture>>& GetAllCache();

	private:

		static std::unordered_map<Engine::GUID, Handle<Texture>> m_cache;
	};
}