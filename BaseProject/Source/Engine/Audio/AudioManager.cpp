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
		return false;
	}
	void AudioManager::Release()
	{
		m_upAudioEngine = nullptr;
	}
	Handle<Resource::SoundInstance> AudioManager::GetSoundInstance(const std::string& a_filePath)
	{
		// サウンドエンジンがなければ発行しない
		if (!m_upAudioEngine) 
		{
			ENGINE_WARNING("サウンドエンジンがない状態でサウンドのリクエストが来ました");
			return Handle<Resource::SoundInstance>();
		}

		// サウンドデータの読込
		auto _sound = Resource::SoundIO::Load(a_filePath);


		return Handle<Resource::SoundInstance>();
	}
	AudioManager::AudioManager()
	{}
	AudioManager::~AudioManager()
	{}
}
