#include "ResourceManager.h"

namespace Engine::Resource
{
	void ResourceManager::Release()
	{

		RunGarbageCollectionSweep();

		// 全プール解放。
		// ここで解放し損ねたリソースはシングルトンの静的破棄まで生き残ってしまい、
		// 破棄順が保証されない他マネージャー(AudioEngine など)を掴んだまま落ちる。
		// リソース型を追加したらここにも必ず足すこと。
		ReleaseData<Model>();
		ReleaseData<Material>();
		ReleaseData<Mesh>();
		ReleaseData<AnimationData>();
		ReleaseData<Texture>();
		ReleaseData<Shader>();
		ReleaseData<AnimatorAsset>();
		ReleaseData<ActionStateMachineAsset>();
		ReleaseData<ParticlesAsset>();
		ReleaseData<ShadingModelTable>();
		ReleaseData<Prefab>();

		// サウンド : DirectX::SoundEffect は AudioEngine を参照しているため、
		// AudioEngine が生きているこのタイミングで必ず解放しきる
		ReleaseData<Sound>();
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