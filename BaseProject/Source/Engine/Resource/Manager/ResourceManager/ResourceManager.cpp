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
		m_animatorAssetData.pool.Release();
		m_actionStateMachineAssetData.pool.Release();

		// サウンド : DirectX::SoundEffect は AudioEngine を参照しているため、
		// AudioEngine が生きているこのタイミングで必ず解放しきる。
		// (静的変数の破棄まで残すと AudioEngine が先に消えて落ちる)
		m_soundData.pool.Release();
	}

	void ResourceManager::AllResetECSRefs()
	{
		ResetECSRefs<Model>();
		ResetECSRefs<Material>();
		ResetECSRefs<Texture>();
		ResetECSRefs<AnimationData>();
		ResetECSRefs<Mesh>();
		ResetECSRefs<Shader>();
		ResetECSRefs<AnimatorAsset>();
		ResetECSRefs<ActionStateMachineAsset>();
	}

	void ResourceManager::RunGarbageCollectionSweep()
	{
		SweepUnused<Model>();
		SweepUnused<Material>();
		SweepUnused<Texture>();
		SweepUnused<Mesh>();
		//SweepUnused<Shader>();
		//SweepUnused<AnimatorAsset>();

		//SweepUnused<AnimationData>();
	}

	ResourceManager::ResourceManager()
	{
		AliveFlag() = true;
	}
	ResourceManager::~ResourceManager()
	{
		// 以降 ResourceRef のデストラクタなどからアクセスされないようにする
		AliveFlag() = false;
	}
}