#include "AudioManager.h"

#include "../Resource/Manager/ResourceManager/ResourceManager.h"

namespace Engine::Audio
{
	bool AudioManager::Init()
	{
		
		Release();

		// エンジンフラグ定義
		DirectX::AUDIO_ENGINE_FLAGS _flgs = DirectX::AudioEngine_EnvironmentalReverb | DirectX::AudioEngine_ReverbUseFilters;
#ifdef _DEBUG
		_flgs |= DirectX::AudioEngine_Debug;
#endif
		// オーディオエンジン作成
		m_upAudioEngine = std::make_unique<DirectX::AudioEngine>(_flgs);
		m_upAudioEngine->SetReverb(DirectX::Reverb_Default);

		m_listener.OrientFront = { 0,0,1 };

		ENGINE_LOG("[Init] AudioManager が初期化されました");
		return true;
	}
	void AudioManager::ReleaseInstances()
	{
		// 鳴っているものを止めてから破棄する
		for (auto& _instance : m_soundInstancePool.RefAll())
		{
			if (!_instance.has_value()) continue;
			_instance->Stop();
		}
		m_soundInstancePool.Release();
	}
	void AudioManager::Release()
	{
		// SoundEffectInstance は AudioEngine を参照しているため、
		// エンジンより先にインスタンスを破棄しきる
		ReleaseInstances();

		m_upAudioEngine = nullptr;
	}
	const Resource::SoundInstance* AudioManager::GetInstance(const Handle<Resource::SoundInstance>& a_handle) const
	{
		return m_soundInstancePool.Get(a_handle);
	}
	Resource::SoundInstance* AudioManager::RefInstance(const Handle<Resource::SoundInstance>& a_handle)
	{
		return m_soundInstancePool.Ref(a_handle);
	}
	Handle<Resource::SoundInstance> AudioManager::RequestSoundInstance(const std::string& a_filePath)
	{
		// ファイルパスの存在チェック
		if(a_filePath.empty()) return Handle<Resource::SoundInstance>();
		auto _guid = Resource::AssetDatabase::Instance().GetGUIDFromFilePath(a_filePath);
		return RequestSoundInstance(_guid);

	}
	Handle<Resource::SoundInstance> AudioManager::RequestSoundInstance(const Engine::GUID& a_guid)
	{
		// サウンドエンジンがなければ発行しない
		if (!m_upAudioEngine)
		{
			ENGINE_WARNING("サウンドエンジンがない状態でサウンドのリクエストが来ました");
			return Handle<Resource::SoundInstance>();
		}

		if(a_guid == Engine::DefaultGUID) return Handle<Resource::SoundInstance>();

		// インスタンスを作成
		Resource::SoundInstance _instance = {};

		// ロード済みのサウンドかチェック
		if (Resource::ResourceManager::Instance().Has<Resource::Sound>(a_guid))
		{
			// サウンドの取得
			auto _soundRef = Resource::ResourceManager::Instance().GetCache<Resource::Sound>(a_guid);
			if (!_soundRef.IsValid()) return Handle<Resource::SoundInstance>();
			_instance.Init(_soundRef);// インスタンスの初期化
			
		}
		else
		{
			// サウンドのロード
			auto _soundRef = Resource::ResourceManager::Instance().Load<Resource::Sound>(a_guid);
			if (!_soundRef.IsValid()) return Handle<Resource::SoundInstance>();
			_instance.Init(_soundRef);// インスタンスの初期化
		}

		return m_soundInstancePool.Add(std::move(_instance));
	}
	AudioManager::AudioManager()
	{}
	AudioManager::~AudioManager()
	{
		// 本来は MainEngine::Release() から明示的に解放されている想定。
		// ここに到達するのは静的変数の破棄フェーズなので、
		// 取りこぼしがあった場合の保険として正しい順序で解放しておく
		Release();
	}
}
