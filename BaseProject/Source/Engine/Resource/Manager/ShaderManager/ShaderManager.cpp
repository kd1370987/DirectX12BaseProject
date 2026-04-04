#include "ShaderManager.h"
namespace Engine::Resource
{
	Resource::Handle<Shader> ShaderManager::Request(const std::string& a_path)
	{
		// 重なり防止
		auto _it = m_handleMap.find(a_path);
		if (_it != m_handleMap.end())
		{
			return _it->second;
		}

		// ハンドル取得
		Resource::Handle<Shader> _handle = m_handleStorage.Allocate();

		// データを入れる
		if (m_shaderVec.size() <= _handle.idx)
		{
			m_shaderVec.resize(_handle.idx + 1);
		}
		m_shaderVec[_handle.idx].Load(a_path);
		m_handleMap[a_path] = _handle;
		return _handle;
	}

	const D3D12_SHADER_BYTECODE& ShaderManager::GetByteCode(const Resource::Handle<Shader>& a_handle) const
	{
		if (m_handleStorage.IsValid(a_handle))
		{
			return m_shaderVec[a_handle.idx].GetByteCode();
		}
		assert(0 && "存在していないシェーダーです");
		return D3D12_SHADER_BYTECODE();
	}
}