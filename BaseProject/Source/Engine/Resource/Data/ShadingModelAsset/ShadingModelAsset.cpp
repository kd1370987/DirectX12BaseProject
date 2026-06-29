#include "ShadingModelAsset.h"
namespace Engine::Resource
{
	std::span<const Handle<Shader>> Engine::Resource::ShadingModelAsset::GetShaderHandles(UINT a_passHash) const
	{
		auto _it = m_shaderHandleMap.find(a_passHash);
		if (_it != m_shaderHandleMap.end())
		{
			return _it->second;
		}
		return {};
	}
	std::span<const Handle<ShaderLibrary>> ShadingModelAsset::GetShaderLibraryHandles(UINT a_passHash) const
	{
		auto _it = m_shaderLibaryHandleMap.find(a_passHash);
		if (_it != m_shaderLibaryHandleMap.end())
		{
			return _it->second;
		}
		return {};
	}
	void ShadingModelAsset::Archive(Persistence::Archive& a_ar)
	{

	}
}