#include "ResourceManager.h"

namespace Engine::Resource
{
	void ResourceManager::Release()
	{

		SweepUnusedAll();

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

		// サウンドの流れ : 中身はGUIDと設定値だけだが、
		// ここに足しておかないとプールが静的破棄まで残る
		ReleaseData<AudioBehavior>();

		// エフェクト : 参照しているパーティクル/モデルより後に解放する
		ReleaseData<EffectAsset>();
	}

	//======================================================================================
	// 使われなくなったリソースを片付ける
	//--------------------------------------------------------------------------------------
	// 参照カウントが 0 のものだけを破棄する。数えているのは実際の持ち主
	// (ResourceRef と、ECS側が Acquire で取った参照)だけで、走査で数え直すことはしない。
	//
	// 呼ぶのはシーンの切れ目(SceneManager::PopScene でシーンが1つも残らなくなったとき)。
	// 参照が 0 になった瞬間に捨てないのは、同じシーンの中で出し直すたびに
	// 読み直しが走るのを避けるため(実質シーン内のキャッシュとして残す)。
	//======================================================================================
	void ResourceManager::SweepUnusedAll()
	{
		// エフェクトが参照しているものより先にエフェクトを片付ける。
		// 先に中身を消すと、参照が残っているのに実体が無い状態を挟んでしまう
		SweepUnused<EffectAsset>();
		SweepUnused<Prefab>();
		SweepUnused<ParticlesAsset>();
		SweepUnused<AudioBehavior>();
		SweepUnused<ActionStateMachineAsset>();
		SweepUnused<AnimatorAsset>();

		// モデルは中身(メッシュ・マテリアル・アニメーション)を ResourceRef で握っているので、
		// モデルが消えた後でないと下は 0 にならない
		SweepUnused<Model>();
		SweepUnused<Material>();
		SweepUnused<Mesh>();
		SweepUnused<AnimationData>();
		SweepUnused<Texture>();

		// シェーダーとシェーディングモデルテーブルは使い回すので片付けない
		// (パスの構築時に引くだけで、持ち主が居ない時間帯がある)
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