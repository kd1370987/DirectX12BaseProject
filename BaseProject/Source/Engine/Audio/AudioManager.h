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

		/// <summary>
		/// 発行済みのサウンドインスタンスをすべて停止・破棄する
		/// DirectX::SoundEffectInstance は生成元の SoundEffect と AudioEngine を
		/// 参照しているため、それらより先にここで破棄しきる必要がある
		/// </summary>
		void ReleaseInstances();

		/// <summary>
		/// 全解放 : サウンドインスタンス → オーディオエンジン の順で破棄する
		/// </summary>
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
		Handle<Resource::SoundInstance> RequestSoundInstance(const std::string& a_filePath);
		Handle<Resource::SoundInstance> RequestSoundInstance(const Engine::GUID& a_guid);

	private:

		// ---- メンバの宣言順が破棄順を決めるので入れ替えないこと ----
		// 破棄は宣言の逆順。SoundInstance が AudioEngine を参照しているため、
		// オーディオエンジンは必ず一番上(= 最後に破棄される位置)に置く。

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