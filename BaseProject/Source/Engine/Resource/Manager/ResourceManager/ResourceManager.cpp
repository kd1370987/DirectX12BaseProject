#include "ResourceManager.h"

namespace Engine::Resource
{
	void ResourceManager::Release()
	{
		// 各プール解放
		m_modelPool.Release();
		m_materialPool.Release();
		m_meshPool.Release();
		m_animationPool.Release();
		m_texturePool.Release();
		m_shaderPool.Release();
		m_shaderLibraryPool.Release();
		m_stateMachinePool.Release();
	}

	ResourceManager::ResourceManager()
	{}
	ResourceManager::~ResourceManager()
	{}
}