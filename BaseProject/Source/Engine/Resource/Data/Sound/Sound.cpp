#include "Sound.h"

#include "../../../Audio/AudioManager.h"

namespace Engine::Resource
{
	SoundInstance::~SoundInstance()
	{
		Stop();
	}
	bool SoundInstance::Init(const ResourceRef<Sound>& a_resourceRef,bool a_is3D)
	{
		// 元データ取得
		auto* _pSound = ResourceManager::Instance().Ref(a_resourceRef);
		if (!_pSound) return false;

		// エフェクト取得
		auto* _pEffect = _pSound->RefSoundEffect();
		if (!_pEffect) return false;

		// インスタンス作成
		DirectX::SOUND_EFFECT_INSTANCE_FLAGS _flags = DirectX::SoundEffectInstance_Default;
		if (a_is3D)
		{
			_flags |= DirectX::SoundEffectInstance_Use3D | DirectX::SoundEffectInstance_ReverbUseFilters;
		}
		m_upSoundInstance = _pEffect->CreateInstance(_flags);
		m_is3D = a_is3D;

		// 元データを参照として保持し、インスタンスが生きている間は
		// Sound(= DirectX::SoundEffect) が解放されないようにする
		m_soundRef = a_resourceRef;

		return true;
	}
	void SoundInstance::Play(bool a_isLoop)
	{
		if (!m_upSoundInstance) return;

		//停止
		m_upSoundInstance->Stop();

		// 再生
		m_upSoundInstance->Play(a_isLoop);
	}
	void SoundInstance::Play3D(const DXSM::Vector3 & a_pos, bool a_isLoop)
	{
		if (!m_upSoundInstance) return;

		if (!m_is3D)
		{
			// 3Dなしで作られたインスタンスに Apply3D を掛けると DirectXTK が例外を投げる。
			// 3Dで鳴らしたい場合は RequestSoundInstance(..., true) で発行すること
			ENGINE_WARNING("3D指定なしで作成されたサウンドインスタンスに Play3D が呼ばれました");
			Play(a_isLoop);
			return;
		}

		m_upSoundInstance->Stop();
		m_upSoundInstance->SetVolume(1);

		m_upSoundInstance->Play(a_isLoop);

		// 座標設定
		m_emitter.SetPosition(a_pos);

		Apply3D();
	}
	void SoundInstance::Apply3D()
	{
		if (!m_upSoundInstance) return;

		// SoundEffectInstance_Use3D なしで作られたインスタンスに対して
		// Apply3D を呼ぶと DirectXTK が例外を投げるため、ここで弾く
		if (!m_is3D) return;

		m_upSoundInstance->Apply3D(Audio::AudioManager::Instance().RefAudioListner(), m_emitter, false);
	}
	void SoundInstance::Stop()
	{
		if (!m_upSoundInstance) return;
		m_upSoundInstance->Stop();
	}
	void SoundInstance::Pause()
	{
		if (!m_upSoundInstance) return;
		m_upSoundInstance->Pause();
	}
	void SoundInstance::Resume()
	{
		if (!m_upSoundInstance) return;
		m_upSoundInstance->Resume();
	}
	void SoundInstance::SetVolume(float a_vol)
	{
		if (!m_upSoundInstance) return;
		m_upSoundInstance->SetVolume(a_vol);
		Apply3D();
	}
	void SoundInstance::SetPos(const DXSM::Vector3 & a_pos)
	{
		if (!m_upSoundInstance) return;
		m_emitter.SetPosition(a_pos);
		Apply3D();
	}
	void SoundInstance::SetCurveDistanceScaler(float a_val)
	{
		if (!m_upSoundInstance) return;
		m_emitter.CurveDistanceScaler = a_val;
		Apply3D();
	}
	bool SoundInstance::IsPlay()
	{
		if (!m_upSoundInstance) return false;
		if (m_upSoundInstance->GetState() == DirectX::SoundState::PLAYING) return true;
		return false;
	}
	bool SoundInstance::IsPause()
	{
		if (!m_upSoundInstance) return false;
		if (m_upSoundInstance->GetState() == DirectX::SoundState::PAUSED) return true;
		return false;
	}
}
