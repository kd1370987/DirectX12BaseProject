#include "SoundFixupSystem.h"

#include "Engine/ECS/World/World.h"

#include "../../../../Components/Tag/SystemPhaseTag/PostDeserializeTag.h"
#include "../../../../Components/Resource/SoundComponent.h"
#include "../../../../../Engine/Audio/AudioManager.h"

void SoundFixupSystem::Init(Engine::ECS::World& a_world)
{
	a_world.PostDeserializeTask<SoundComponent>(
		Engine::ECS::ESystemType::PostDeserialize,
		"SoundFixupSystem",
		[]
		(
			Engine::ECS::ArchetypeChunk* a_pChunk,
			uint32_t a_count,
			const Engine::ECS::SystemContext& a_ctx,
			PostDeserializeTag* a_tag,
			SoundComponent* a_soundArray
			)
		{
			auto* _pAudioManager = a_ctx.pServices->pAudioManager;
			if (!_pAudioManager) return;

			for (size_t _i = 0; _i < a_count; ++_i)
			{
				SoundComponent& _soundComp = a_soundArray[_i];

				// エディターでサウンドを差し替えた場合、リフレッシュで
				// Release → PostDeserialize と流れて古いインスタンスは返却済み。
				// それ以外の経路で残っていた場合の二重発行を防ぐ
				if (_soundComp.soundInstanceHandle.IsValid())
				{
					_pAudioManager->ReleaseSoundInstance(_soundComp.soundInstanceHandle);
					_soundComp.soundInstanceHandle = {};
				}

				if (_soundComp.soundGUID == Engine::DefaultGUID) continue;

				// GUIDからサウンドをロードして再生用インスタンスを発行
				_soundComp.soundInstanceHandle = _pAudioManager->RequestSoundInstance(_soundComp.soundGUID);

				// 保存されていた音量を反映
				if (auto* _pInstance = _pAudioManager->RefInstance(_soundComp.soundInstanceHandle))
				{
					_pInstance->SetVolume(_soundComp.vol);
				}
			}
		}
	);
}
