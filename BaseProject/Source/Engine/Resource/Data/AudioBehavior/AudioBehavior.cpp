#include "AudioBehavior.h"

#include "../../../Audio/AudioManager.h"

namespace Engine::Resource
{
	const char* ToString(EAudioPhase a_phase)
	{
		switch (a_phase)
		{
		case EAudioPhase::Start:	return "Start";
		case EAudioPhase::Loop:		return "Loop";
		case EAudioPhase::End:		return "End";
		default:					return "Unknown";
		}
	}

	//======================================================================================
	// SoundPart
	//======================================================================================
	void SoundPart::Archive(Persistence::Archive& a_ar)
	{
		a_ar.Field("SoundGUID", soundGUID);
		a_ar.Field("Vol", vol);
		a_ar.Field("Is3DSound", is3DSound);
	}

	//======================================================================================
	// AudioBehaviorInstance
	//======================================================================================
	void AudioBehaviorInstance::StopAll(Engine::Audio::AudioManager& a_audioManager)
	{
		for (size_t _i = 0; _i < AUDIO_PHASE_COUNT; ++_i)
		{
			auto* _pInstance = a_audioManager.RefInstance(handles[_i]);
			if (!_pInstance) continue;
			_pInstance->Stop();
		}

		isActive = false;
	}

	void AudioBehaviorInstance::SetPos(Engine::Audio::AudioManager& a_audioManager, const Math::Vector3& a_pos)
	{
		pos = a_pos;

		for (size_t _i = 0; _i < AUDIO_PHASE_COUNT; ++_i)
		{
			auto* _pInstance = a_audioManager.RefInstance(handles[_i]);
			if (!_pInstance) continue;

			// 2Dで発行されたインスタンスに位置を渡すと DirectXTK が例外を投げる
			if (!_pInstance->Is3D()) continue;

			_pInstance->SetPos(pos);
		}
	}

	void AudioBehaviorInstance::Release(Engine::Audio::AudioManager& a_audioManager)
	{
		for (size_t _i = 0; _i < AUDIO_PHASE_COUNT; ++_i)
		{
			// 鳴っていても止めて返却される
			a_audioManager.ReleaseSoundInstance(handles[_i]);
			handles[_i] = {};
		}

		isActive = false;
	}

	//======================================================================================
	// AudioBehavior
	//======================================================================================
	void AudioBehavior::Archive(Persistence::Archive& a_ar)
	{
		a_ar.StringField("Name", m_name);

		// フェーズは固定数なので、要素数は書かずに順番だけで並べる。
		// (フェーズを増やすとバイナリの並びが変わるので、
		//  既存の .ob* は作り直しが必要になる)
		for (size_t _i = 0; _i < AUDIO_PHASE_COUNT; ++_i)
		{
			const auto _phase = static_cast<EAudioPhase>(_i);
			if (a_ar.BeginGroup(ToString(_phase)))
			{
				m_parts[_i].Archive(a_ar);
				a_ar.EndGroup();
			}
		}
	}

	void AudioBehavior::Save(const std::string& a_baseFilePath)
	{
		auto _fileDir = Engine::File::GetDirFromPath(a_baseFilePath);
		auto _fileName = Engine::File::GetFileNameWithoutExtension(a_baseFilePath);

		Persistence::Archive _ar(Persistence::Archive::Mode::Save, _fileDir, _fileName, "audbhv");
		Archive(_ar);
	}

	void AudioBehavior::CreateInstance(Engine::Audio::AudioManager& a_audioManager, AudioBehaviorInstance& a_inst) const
	{
		// 作り直しでも漏らさないよう、まず持っているものを返す
		a_inst.Release(a_audioManager);

		for (size_t _i = 0; _i < AUDIO_PHASE_COUNT; ++_i)
		{
			const SoundPart& _part = m_parts[_i];

			// 未設定のフェーズは無効ハンドルのままにしておく。
			// 鳴らす側はハンドルが引けないので自然に飛ばされる
			if (!_part.IsValid()) continue;

			a_inst.handles[_i] = a_audioManager.RequestSoundInstance(_part.soundGUID, _part.is3DSound);

			// 設定した音量を反映しておく
			if (auto* _pInstance = a_audioManager.RefInstance(a_inst.handles[_i]))
			{
				_pInstance->SetVolume(_part.vol);
			}
		}

		a_inst.isActive = false;
	}

	bool AudioBehavior::PlayPart(
		Engine::Audio::AudioManager& a_audioManager,
		AudioBehaviorInstance& a_inst,
		EAudioPhase a_phase,
		bool a_isLoop
	) const
	{
		const SoundPart& _part = GetPart(a_phase);
		if (!_part.IsValid()) return false;

		auto* _pInstance = a_audioManager.RefInstance(a_inst.GetHandle(a_phase));
		if (!_pInstance) return false;

		if (_part.is3DSound)
		{
			_pInstance->Play3D(a_inst.pos, a_isLoop);

			// Play3D は音量を 1 に戻すので、鳴らした後に入れ直す
			_pInstance->SetVolume(_part.vol);
		}
		else
		{
			// Play は内部で Stop してから鳴らすので、連打しても頭から鳴り直す
			_pInstance->Play(a_isLoop);
		}

		return true;
	}

	void AudioBehavior::Start(Engine::Audio::AudioManager& a_audioManager, AudioBehaviorInstance& a_inst) const
	{
		PlayPart(a_audioManager, a_inst, EAudioPhase::Start, false);

		// 始動音を入れていなくても「動き出した」ことは覚えておく。
		// (始動音なし・終了音ありという組み合わせでも End が鳴るようにするため)
		a_inst.isActive = true;
	}

	void AudioBehavior::Loop(Engine::Audio::AudioManager& a_audioManager, AudioBehaviorInstance& a_inst) const
	{
		auto* _pInstance = a_audioManager.RefInstance(a_inst.GetHandle(EAudioPhase::Loop));

		// 継続音が鳴りっぱなしになるものなので、
		// 止まっているときだけ鳴らし直せば二重再生にならない。
		// 継続音を入れていない場合は何も鳴らさず、状態だけ進める
		if (_pInstance && !_pInstance->IsPlay())
		{
			PlayPart(a_audioManager, a_inst, EAudioPhase::Loop, true);
		}

		a_inst.isActive = true;
	}

	void AudioBehavior::End(Engine::Audio::AudioManager& a_audioManager, AudioBehaviorInstance& a_inst) const
	{
		// 動いていないなら、すでに終わっている。
		// (止まっている間も毎フレーム End が呼ばれる作りなので、
		//  ここで抜けないと終了音が鳴り続ける)
		if (!a_inst.isActive) return;

		a_inst.isActive = false;

		if (auto* _pLoop = a_audioManager.RefInstance(a_inst.GetHandle(EAudioPhase::Loop)))
		{
			_pLoop->Stop();
		}

		PlayPart(a_audioManager, a_inst, EAudioPhase::End, false);
	}
}
