#include "AudioManager.h"

#include "../Resource/Manager/ResourceManager/ResourceManager.h"

namespace Engine::Audio
{
	//======================================================================================
	// 音量
	//======================================================================================
	void AudioManager::SetMasterVolume(float a_volume)
	{
		const float _volume = std::clamp(a_volume, 0.0f, 1.0f);
		if (m_masterVolume == _volume) return;

		m_masterVolume = _volume;
		RefreshAllVolume();
	}

	void AudioManager::SetGroupVolume(ESoundGroup a_group, float a_volume)
	{
		const size_t _index = static_cast<size_t>(a_group);
		if (_index >= m_groupVolumeArray.size()) return;

		const float _volume = std::clamp(a_volume, 0.0f, 1.0f);
		if (m_groupVolumeArray[_index] == _volume) return;

		m_groupVolumeArray[_index] = _volume;
		RefreshAllVolume();
	}

	float AudioManager::GetGroupVolume(ESoundGroup a_group) const
	{
		const size_t _index = static_cast<size_t>(a_group);
		if (_index >= m_groupVolumeArray.size()) return 1.0f;

		return m_groupVolumeArray[_index];
	}

	float AudioManager::CalcVolumeScale(ESoundGroup a_group) const
	{
		return m_masterVolume * GetGroupVolume(a_group);
	}

	//======================================================================================
	// 鳴っているものすべてへ音量を送り直す
	//--------------------------------------------------------------------------------------
	// 鳴らしている側は設定が変わったことに気付けない
	// (重ねたシーンのように更新が止まっているものもある)ので、ここから送り込む
	//======================================================================================
	void AudioManager::RefreshAllVolume()
	{
		for (auto& _instance : m_soundInstancePool.RefAll())
		{
			if (!_instance.has_value()) continue;
			_instance->RefreshVolume();
		}
	}

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
	void AudioManager::Update()
	{
		if (!m_upAudioEngine) return;

		// Update() が false を返すのは「無音」か「デバイスロスト」のどちらか
		if (m_upAudioEngine->Update()) return;

		if (!m_upAudioEngine->IsCriticalError()) return;

		// オーディオデバイスが失われたので作り直す。
		// 発行済みの SoundEffectInstance は AudioEngine 側で新デバイスへ移行される
		ENGINE_WARNING("オーディオデバイスをロストしました。再初期化します");
		if (!m_upAudioEngine->Reset())
		{
			ENGINE_WARNING("オーディオデバイスの再初期化に失敗しました");
		}
	}
	void AudioManager::SubmitListener(const ListenerData& a_data)
	{
		m_listener.SetPosition(a_data.pos);
		m_listener.SetVelocity(a_data.velocity);

		// X3DAudio は前方・上方向が「正規化されていて直交している」ことを前提にしている。
		// 送られてきた行列にスケールや微妙な歪みが乗っていても落ちないよう、ここで整える。
		DXSM::Vector3 _front = a_data.front;
		if (_front.LengthSquared() > 1e-8f) _front.Normalize();
		else                                _front = DXSM::Vector3(0.0f, 0.0f, 1.0f);

		// 上方向から前方成分を抜いて直交化する
		DXSM::Vector3 _up = a_data.up - _front * a_data.up.Dot(_front);
		if (_up.LengthSquared() <= 1e-8f)
		{
			// 前方と上方向が平行だった場合の逃げ道。前方と重ならない軸から作り直す
			DXSM::Vector3 _ref = (std::fabs(_front.y) > 0.99f)
				? DXSM::Vector3(0.0f, 0.0f, 1.0f)
				: DXSM::Vector3(0.0f, 1.0f, 0.0f);
			_up = _ref - _front * _ref.Dot(_front);
		}
		_up.Normalize();

		m_listener.SetOrientation(_front, _up);
	}
	const Resource::SoundInstance* AudioManager::GetInstance(const Handle<Resource::SoundInstance>& a_handle) const
	{
		return m_soundInstancePool.Get(a_handle);
	}
	Resource::SoundInstance* AudioManager::RefInstance(const Handle<Resource::SoundInstance>& a_handle)
	{
		return m_soundInstancePool.Ref(a_handle);
	}
	Handle<Resource::SoundInstance> AudioManager::RequestSoundInstance(
		const std::string& a_filePath, bool a_is3D, ESoundGroup a_group)
	{
		// ファイルパスの存在チェック
		if(a_filePath.empty()) return Handle<Resource::SoundInstance>();
		auto _guid = Resource::AssetDatabase::Instance().GetGUIDFromFilePath(a_filePath);
		return RequestSoundInstance(_guid, a_is3D, a_group);

	}
	Handle<Resource::SoundInstance> AudioManager::RequestSoundInstance(
		const Engine::GUID& a_guid, bool a_is3D, ESoundGroup a_group)
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
			_instance.Init(_soundRef, a_is3D);// インスタンスの初期化

		}
		else
		{
			// サウンドのロード
			auto _soundRef = Resource::ResourceManager::Instance().LoadImmediate<Resource::Sound>(a_guid);
			if (!_soundRef.IsValid()) return Handle<Resource::SoundInstance>();
			_instance.Init(_soundRef, a_is3D);// インスタンスの初期化
		}

		// グループの札を付けてから預ける。
		// 以降この音は、そのグループの音量とマスター音量が掛かった状態で鳴る
		_instance.SetGroup(a_group);

		return m_soundInstancePool.Add(std::move(_instance));
	}
	void AudioManager::ReleaseSoundInstance(const Handle<Resource::SoundInstance>& a_handle)
	{
		// 存在しないハンドルは Remove 側で弾かれるので、ここでは停止だけ気にする
		if (auto* _pInstance = m_soundInstancePool.Ref(a_handle))
		{
			_pInstance->Stop();
		}
		m_soundInstancePool.Remove(a_handle);
	}
	AudioManager::AudioManager()
	{
		// グループ音量は既定で素通し。
		// 実際の値は起動時にオプション(AudioOption)から流し込まれる
		m_groupVolumeArray.fill(1.0f);
	}
	AudioManager::~AudioManager()
	{
		// 本来は MainEngine::Release() から明示的に解放されている想定。
		// ここに到達するのは静的変数の破棄フェーズなので、
		// 取りこぼしがあった場合の保険として正しい順序で解放しておく
		Release();
	}
}
