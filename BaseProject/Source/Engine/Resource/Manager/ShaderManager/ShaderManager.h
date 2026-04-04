#pragma once

namespace Engine::Resource
{
	class ShaderManager
	{
	public:

		// シェーダーのリクエスト : なければ登録して返す
		Resource::Handle<Shader> Request(const std::string& a_path);
		
		// アクセサ 
		const D3D12_SHADER_BYTECODE& GetByteCode(const Resource::Handle<Shader>& a_handle) const;

		
	private:

		// 重なり防止用
		std::unordered_map<std::string, Resource::Handle<Shader>> m_handleMap;

		Storage::HandleStorage<Shader> m_handleStorage;
		std::vector<Shader> m_shaderVec;
	};
}