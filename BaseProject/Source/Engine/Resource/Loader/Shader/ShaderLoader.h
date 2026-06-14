#pragma once
namespace Engine::Resource
{
	class ShaderLoader
	{
	public:

		// 読み込み
		static Handle<Shader> Load(const Engine::GUID& a_guid);
		static Handle<ShaderLibrary> LoadShaderLibrary(const Engine::GUID& a_guid);

		// リクエスト
		static Handle<Shader> Request(const std::string& a_path);
		static Handle<ShaderLibrary> RequestShaderLibrary(const std::string& a_path);

		// アクセサ
		static const std::unordered_map<Engine::GUID, Handle<Shader>>& GetAllShaderCache();
		static const std::unordered_map<Engine::GUID, Handle<ShaderLibrary>>& GetAllShaderLibraryCache();

		static bool HasShader(const Engine::GUID& a_guid);
		static bool HasShaderLibrary(const Engine::GUID& a_guid);

	private:

		static std::unordered_map<Engine::GUID, Handle<Shader>>			m_shaderCache;
		static std::unordered_map<Engine::GUID, Handle<ShaderLibrary>>	m_shaderLibraryCache;
	};
}