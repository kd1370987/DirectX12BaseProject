#include "ResourceManager.h"

namespace Engine::Resource
{
	void ResourceManager::Release()
	{

		RunGarbageCollectionSweep();

		// 各プール解放
		m_modelData.pool.Release();
		m_materialData.pool.Release();
		m_meshData.pool.Release();
		m_animationData.pool.Release();
		m_textureData.pool.Release();
		m_shaderData.pool.Release();
		m_shaderLibraryData.pool.Release();
		m_stateMachineAssetData.pool.Release();
	}

	void ResourceManager::RunGarbageCollectionSweep()
	{
		SweepUnused<Model>();
		SweepUnused<Material>();
		SweepUnused<Texture>();
		SweepUnused<AnimationData>();
		SweepUnused<Mesh>();
		SweepUnused<Shader>();
		SweepUnused<ShaderLibrary>();
		SweepUnused<StateMachineAsset>();
	}

	ResourceManager::ResourceManager()
	{}
	ResourceManager::~ResourceManager()
	{}
}