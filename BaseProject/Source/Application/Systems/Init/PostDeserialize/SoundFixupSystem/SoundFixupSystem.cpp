#include "SoundFixupSystem.h"

#include "Engine/ECS/World/World.h"

#include "../../../../Components/Tag/SystemPhaseTag/PostDeserializeTag.h"
#include "../../../../Components/Resource/SoundComponent.h"
#include "../../../../Components/Resource/HitSoundComponent.h"
#include "../../../../Components/Resource/AudioBehaviorComponent.h"
#include "../../../../../Engine/Audio/AudioManager.h"

//==========================================================================================
// SoundFixupSystem
//
// GUID しか保存されていないサウンドから、再生用インスタンスを発行し直す。
// サウンドを持つコンポーネントごとにタスクを登録する
// (同じ寿命の管理なので、システムを分けずここにまとめている)。
//
// AudioBehaviorComponent はファイル1つではなく「音の流れ」を指すので、
// まずアセットを解決してから、その定義に沿ってフェーズぶんのインスタンスを発行する。
//==========================================================================================
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

	// 被弾音。SoundComponent とまったく同じ発行の流れ
	a_world.PostDeserializeTask<HitSoundComponent>(
		Engine::ECS::ESystemType::PostDeserialize,
		"HitSoundFixupSystem",
		[]
		(
			Engine::ECS::ArchetypeChunk* a_pChunk,
			uint32_t a_count,
			const Engine::ECS::SystemContext& a_ctx,
			PostDeserializeTag* a_tag,
			HitSoundComponent* a_hitSoundArray
			)
		{
			auto* _pAudioManager = a_ctx.pServices->pAudioManager;
			if (!_pAudioManager) return;

			for (size_t _i = 0; _i < a_count; ++_i)
			{
				HitSoundComponent& _hitSoundComp = a_hitSoundArray[_i];

				// 二重発行を防ぐ(リフレッシュ経路では返却済み)
				if (_hitSoundComp.soundInstanceHandle.IsValid())
				{
					_pAudioManager->ReleaseSoundInstance(_hitSoundComp.soundInstanceHandle);
					_hitSoundComp.soundInstanceHandle = {};
				}

				// 鳴らす条件は作り直しでリセットしておく
				_hitSoundComp.coolTime = 0.0f;

				if (_hitSoundComp.soundGUID == Engine::DefaultGUID) continue;

				_hitSoundComp.soundInstanceHandle = _pAudioManager->RequestSoundInstance(_hitSoundComp.soundGUID);

				if (auto* _pInstance = _pAudioManager->RefInstance(_hitSoundComp.soundInstanceHandle))
				{
					_pInstance->SetVolume(_hitSoundComp.vol);
				}
			}
		}
	);

	// オーディオビヘイビア。
	// アセット側に音量と3D指定まで入っているので、ここでは発行するだけでよい
	a_world.PostDeserializeTask<AudioBehaviorComponent>(
		Engine::ECS::ESystemType::PostDeserialize,
		"AudioBehaviorFixupSystem",
		[]
		(
			Engine::ECS::ArchetypeChunk* a_pChunk,
			uint32_t a_count,
			const Engine::ECS::SystemContext& a_ctx,
			PostDeserializeTag* a_tag,
			AudioBehaviorComponent* a_behaviorArray
			)
		{
			auto* _pAudioManager = a_ctx.pServices->pAudioManager;
			auto* _pResourceManager = a_ctx.pServices->pResourceManager;
			if (!_pAudioManager || !_pResourceManager) return;

			for (size_t _i = 0; _i < a_count; ++_i)
			{
				AudioBehaviorComponent& _behaviorComp = a_behaviorArray[_i];

				// 差し替え時のリフレッシュでは Release 済みだが、
				// それ以外の経路で残っていた場合の二重発行を防ぐ
				_behaviorComp.instance.Release(*_pAudioManager);

				if (_behaviorComp.behaviorGUID == Engine::DefaultGUID)
				{
					_behaviorComp.behaviorHandle = {};
					continue;
				}

				// ビヘイビアアセットを解決
				_behaviorComp.behaviorHandle =
					_pResourceManager->LoadImmediate<Engine::Resource::AudioBehavior>(_behaviorComp.behaviorGUID);

				auto* _pBehavior = _pResourceManager->Ref(_behaviorComp.behaviorHandle);
				if (!_pBehavior) continue;

				// 定義に沿ってフェーズぶんの再生用インスタンスを発行する。
				// 音が入っていないフェーズは無効ハンドルのまま残る
				_pBehavior->CreateInstance(*_pAudioManager, _behaviorComp.instance);
			}
		}
	);
}
