#pragma once

namespace Engine::Audio
{
	/// <summary>
	/// 音関係を扱うシングルトンクラス、サウンドはここで読込要求が来るとロードしてインスタンスのみを返す
	/// </summary>
	class AudioManager
	{
	public:
		//----------------------------------------------------------------------------------------------------
		// 初期化・解放
		//----------------------------------------------------------------------------------------------------
		bool Init();
		void Release();

		//----------------------------------------------------------------------------------------------------
		// アクセサ
		//----------------------------------------------------------------------------------------------------
		DirectX::AudioEngine* RefAudioEngine() { return m_upAudioEngine.get(); }
		DirectX::AudioListener& RefAudioListner() { return m_listener; }

		const Resource::SoundInstance* GetInstance(const Handle<Resource::SoundInstance>& a_handle) const;
		Resource::SoundInstance* RefInstance(const Handle<Resource::SoundInstance>& a_handle);

		//----------------------------------------------------------------------------------------------------
		// サウンドインスタンスの発行
		//----------------------------------------------------------------------------------------------------
		Handle<Resource::SoundInstance> GetSoundInstance(const std::string& a_filePath);
		Handle<Resource::SoundInstance> GetSoundInstance(const Engine::GUID& a_guid);

	private:

		// オーディオエンジン
		std::unique_ptr<DirectX::AudioEngine> m_upAudioEngine = nullptr;

		// 3Dサウンドリスナー
		DirectX::AudioListener m_listener;

		// 現在再生中のサウンド管理リスト
		Pool::ItemPool<Resource::SoundInstance> m_soundInstancePool;

	// シングルトン
	private:

		AudioManager();
		~AudioManager();
		NON_COPYABLE_NON_MOVABLE(AudioManager);

	public:

		static AudioManager& Instance()
		{
			static AudioManager _instance;
			return _instance;
		}

	};
}