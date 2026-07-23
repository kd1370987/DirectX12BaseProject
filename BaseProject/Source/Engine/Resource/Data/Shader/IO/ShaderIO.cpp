#include "ShaderIO.h"

#include "../../../Manager/AssetDatabase/AssetDatabase.h"
#include "../../../Manager/ResourceManager/ResourceManager.h"
namespace Engine::Resource
{
	Shader ShaderIO::LoadShaderFromFile(const std::string& a_path)
	{
		// なければロード
		Shader _shader = {};
		_shader.Load(a_path);
		return _shader;
	}

	Handle<Shader> Engine::Resource::ShaderIO::Request(const std::string& a_path)
	{
		// アセットデータベースに問い合わせ
		auto _guid = Resource::AssetDatabase::Instance().GetGUIDFromFilePath(a_path);
		if (_guid != Engine::DefaultGUID)
		{
			// 見つかれば
			auto _shader = LoadShaderFromFile(a_path);
			return ResourceManager::Instance().AddResourceAndGUID(std::move(_shader), _guid);
		}

		ENGINE_LOG("シェーダーの読み込みに失敗しました : %s",a_path.c_str());
		return Handle<Shader>();
	}
}