#include "ShaderLoader.h"

#include "../../Manager/AssetDatabase/AssetDatabase.h"
#include "../../Manager/ResourceManager/ResourceManager.h"
namespace Engine::Resource
{
	Shader ShaderLoader::LoadShaderFromFile(const std::string& a_path)
	{
		// なければロード
		Shader _shader = {};
		_shader.Load(a_path);
		return _shader;
	}

	ShaderLibrary ShaderLoader::LoadShaderLibraryFromFile(const std::string& a_path)
	{
		// 読み込みチェック
		ShaderLibrary _shader = {};
		_shader.Load(a_path);
		return _shader;
	}

	Handle<Shader> Engine::Resource::ShaderLoader::Request(const std::string& a_path)
	{
		// アセットデータベースに問い合わせ
		auto _guid = Resource::AssetDatabase::Instance().GetGUIDFromFilePath(a_path);
		if (_guid != Engine::DefaultGUID)
		{
			// 見つかれば
			auto _shader = LoadShaderFromFile(a_path);
			return ResourceManager::Instance().AddResourceAndGUID(std::move(_shader), _guid);
		}

		return Handle<Shader>();
	}

	Handle<ShaderLibrary> Engine::Resource::ShaderLoader::RequestShaderLibrary(const std::string& a_path)
	{
		// アセットデータベースに問い合わせ
		auto _guid = Resource::AssetDatabase::Instance().GetGUIDFromFilePath(a_path);
		if (_guid != Engine::DefaultGUID)
		{
			// 見つかれば
			auto _shader = LoadShaderLibraryFromFile(a_path);
			return ResourceManager::Instance().AddResourceAndGUID(std::move(_shader), _guid);
		}

		return Handle<ShaderLibrary>();
	}
}