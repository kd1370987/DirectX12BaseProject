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
		// フォント : アトラステクスチャを ResourceRef で握っているのでテクスチャより先に捨てる
		ReleaseData<Font>();

		ReleaseData<Model>();
		ReleaseData<Material>();
		ReleaseData<Mesh>();
		ReleaseData<AnimationData>();
		ReleaseData<Texture>();
		ReleaseData<Shader>();
		ReleaseData<AnimatorAsset>();
		ReleaseData<ParticlesAsset>();
		ReleaseData<Prefab>();
		ReleaseData<EffectPrefab>();

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
		SweepUnused<EffectPrefab>();
		SweepUnused<ParticlesAsset>();
		SweepUnused<AudioBehavior>();
		SweepUnused<AnimatorAsset>();

		// フォントもアトラステクスチャを ResourceRef で握っているので、テクスチャより先に片付ける
		SweepUnused<Font>();

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

	//======================================================================================
	// 型を知らないままの読み込み要求
	//--------------------------------------------------------------------------------------
	// 種別の文字列は MainEngine がアセットデータベースへ登録しているもの。
	// 種別を足したらここにも足すこと(足さないと先読み一覧に載っても読まれない)。
	//
	// ジョブへ流すのは、ほかの場所でも非同期に読まれている種別だけにしてある。
	// それ以外は今まで呼び出しスレッドでしか読まれてこなかったので、
	// ワーカーから読んで大丈夫かを確かめていない
	//======================================================================================
	bool ResourceManager::RequestLoadByGUID(const Core::GUID& a_guid)
	{
		// 実体はデータベースの持ち物で、監視スレッドに消されうるので種別は写しておく
		std::string _type = {};
		if (const AssetProperty* _pProp = m_upAssetDatabase->FindAssetProperty(a_guid))
		{
			_type = _pProp->type;
		}
		if (_type.empty()) return false;

		// ---- ジョブへ流す : 待たない ----
		if (_type == "Model")			{ RequestLoad<Model>(a_guid);			return true; }
		if (_type == "Texture")			{ RequestLoad<Texture>(a_guid);			return true; }
		if (_type == "EffectAsset")		{ RequestLoad<EffectAsset>(a_guid);		return true; }
		if (_type == "Font")			{ RequestLoad<Font>(a_guid);			return true; }

		// ---- その場で読み切る ----
		if (_type == "Mesh")			{ LoadImmediate<Mesh>(a_guid);				return true; }
		if (_type == "Material")		{ LoadImmediate<Material>(a_guid);			return true; }
		if (_type == "Animation")		{ LoadImmediate<AnimationData>(a_guid);		return true; }
		if (_type == "AnimatorAsset")	{ LoadImmediate<AnimatorAsset>(a_guid);		return true; }
		if (_type == "ParticlesAsset")	{ LoadImmediate<ParticlesAsset>(a_guid);	return true; }
		if (_type == "Shader")			{ LoadImmediate<Shader>(a_guid);			return true; }
		if (_type == "Sound")			{ LoadImmediate<Sound>(a_guid);				return true; }
		if (_type == "AudioBehavior")	{ LoadImmediate<AudioBehavior>(a_guid);		return true; }
		if (_type == "RenderingPipelineAsset") { LoadImmediate<Graphics::Pipeline::RenderingPipelineAsset>(a_guid); return true; }

		// プレハブはワールドを引きながら読む : 呼び出し元のシーンのワールドを掴ませる
		if (_type == "Prefab")			{ LoadImmediate<Prefab>(a_guid);			return true; }
		if (_type == "EffectPrefab")	{ LoadImmediate<EffectPrefab>(a_guid);		return true; }

		// シーンなど、このマネージャーが持たない種別
		return false;
	}

	//======================================================================================
	// 読み込み要求の記録
	//======================================================================================
	void ResourceManager::BeginRecordRequests()
	{
		std::lock_guard _lock(m_recordMutex);

		if (m_isRecordingRequests.load(std::memory_order_acquire))
		{
			ENGINE_WARNING("[Resource] 読み込み要求の記録が二重に始められました。前の記録は捨てます");
		}

		m_recordedGUIDVec.clear();
		m_recordedGUIDSet.clear();
		m_isRecordingRequests.store(true, std::memory_order_release);
	}

	std::vector<Core::GUID> ResourceManager::EndRecordRequests()
	{
		std::lock_guard _lock(m_recordMutex);

		m_isRecordingRequests.store(false, std::memory_order_release);

		std::vector<Core::GUID> _result = std::move(m_recordedGUIDVec);
		m_recordedGUIDVec.clear();
		m_recordedGUIDSet.clear();
		return _result;
	}

	void ResourceManager::RecordRequest(const Core::GUID& a_guid)
	{
		std::lock_guard _lock(m_recordMutex);

		// 記録中かを見てからロックを取るまでの間に止められていたら数えない
		if (!m_isRecordingRequests.load(std::memory_order_acquire)) return;

		if (m_recordedGUIDSet.insert(a_guid).second)
		{
			m_recordedGUIDVec.push_back(a_guid);
		}
	}

	ResourceManager::ResourceManager()
		: m_upAssetDatabase(std::make_unique<AssetDatabase>())
	{
		ENGINE_ERRLOG(s_pInstance == nullptr, "ResourceManager は1つだけ作ること");
		s_pInstance = this;
		AliveFlag() = true;
	}
	ResourceManager::~ResourceManager()
	{
		// 以降 ResourceRef のデストラクタなどからアクセスされないようにする。
		// メンバ(プール)が壊れるのはこの本体を抜けた後で、そこで中身の ResourceRef が
		// 参照を返しに来るが、生存フラグを見て引き返す
		AliveFlag() = false;
		s_pInstance = nullptr;
	}
}