#include "ShaderLoader.h"

#include "../../Manager/AssetDatabase/AssetDatabase.h"
#include "../../Manager/ResourceManager/ResourceManager.h"
namespace Engine::Resource
{
	std::unordered_map<Engine::GUID, Engine::Resource::Handle<Engine::Resource::Shader>>			Engine::Resource::ShaderLoader::m_shaderCache;
	std::unordered_map<Engine::GUID, Engine::Resource::Handle<Engine::Resource::ShaderLibrary>>		Engine::Resource::ShaderLoader::m_shaderLibraryCache;

	Handle<Shader> Engine::Resource::ShaderLoader::Load(const Engine::GUID& a_guid)
	{
		// 読み込みチェック
		auto _it = m_shaderCache.find(a_guid);
		if (_it != m_shaderCache.end())
		{
			return _it->second;
		}

		// なければロード
		auto _path = AssetDatabase::Instance().GetFilePathFromGUID(a_guid);
		Shader _shader = {};
		_shader.Load(_path);

		// リソースマネージャーに登録
		auto _handle = ResourceManager::Instance().Add(std::move(_shader));

		// キャッシュに登録
		m_shaderCache.emplace(a_guid,_handle);

		return _handle;
	}

	Handle<ShaderLibrary> Engine::Resource::ShaderLoader::LoadShaderLibrary(const Engine::GUID& a_guid)
	{
		// 読み込みチェック
		auto _it = m_shaderLibraryCache.find(a_guid);
		if (_it != m_shaderLibraryCache.end())
		{
			return _it->second;
		}

		// なければロード
		auto _path = AssetDatabase::Instance().GetFilePathFromGUID(a_guid);
		ShaderLibrary _shader = {};
		_shader.Load(_path);

		// リソースマネージャーに登録
		auto _handle = ResourceManager::Instance().Add(std::move(_shader));

		// キャッシュに登録
		m_shaderLibraryCache.emplace(a_guid, _handle);

		return _handle;
	}

	Handle<Shader> Engine::Resource::ShaderLoader::Request(const std::string& a_path)
	{
		// アセットデータベースに問い合わせ
		auto _guid = Resource::AssetDatabase::Instance().GetGUIDFromFilePath(a_path);
		if (_guid != Engine::DefaultGUID)
		{
			return Load(_guid);
		}

		return Handle<Shader>();
	}

	Handle<ShaderLibrary> Engine::Resource::ShaderLoader::RequestShaderLibrary(const std::string& a_path)
	{
		// アセットデータベースに問い合わせ
		auto _guid = Resource::AssetDatabase::Instance().GetGUIDFromFilePath(a_path);
		if (_guid != Engine::DefaultGUID)
		{
			return LoadShaderLibrary(_guid);
		}

		return Handle<ShaderLibrary>();
	}

	const std::unordered_map<Engine::GUID, Handle<Shader>>& Engine::Resource::ShaderLoader::GetAllShaderCache()
	{
		return m_shaderCache;
	}

	const std::unordered_map<Engine::GUID, Handle<ShaderLibrary>>& Engine::Resource::ShaderLoader::GetAllShaderLibraryCache()
	{
		return m_shaderLibraryCache;
	}
	bool ShaderLoader::HasShader(const Engine::GUID& a_guid)
	{
		auto _it = m_shaderCache.find(a_guid);
		if (_it != m_shaderCache.end())
		{
			return true;
		}
		return false;
	}
	bool ShaderLoader::HasShaderLibrary(const Engine::GUID& a_guid)
	{
		auto _it = m_shaderLibraryCache.find(a_guid);
		if (_it != m_shaderLibraryCache.end())
		{
			return true;
		}
		return false;
	}
}