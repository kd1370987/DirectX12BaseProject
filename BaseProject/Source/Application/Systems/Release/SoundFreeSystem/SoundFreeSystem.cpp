#include "SoundFreeSystem.h"

#include "Application/ECS/World/World.h"

#include "../../../Components/Resource/SoundComponent.h"
#include "../../../Components/Resource/HitSoundComponent.h"
#include "../../../Components/Resource/AudioBehaviorComponent.h"
#include "../../../Components/Effect/EffectAssetComponent.h"
#include "../../../../Engine/Audio/AudioManager.h"

void SoundFreeSystem::Init(App::ECS::World& a_world)
{
	a_world.ReleaseTask<SoundComponent>(
		Engine::ECS::ESystemType::Release,
		"SoundFreeSystem",
		[]
		(
			Engine::ECS::ArchetypeChunk* a_pChunk,
			uint32_t a_count,
			const Engine::ECS::SystemContext& a_ctx,
			ReleaseTag* a_releaseTag,
			SoundComponent* a_soundArray
			)
		{
			auto* _pAudioManager = a_ctx.pServices->pAudioManager;
			if (!_pAudioManager) return;

			for (size_t _i = 0; _i < a_count; ++_i)
			{
				SoundComponent& _soundComp = a_soundArray[_i];

				// 鳴っていても止めて返却される
				_pAudioManager->ReleaseSoundInstance(_soundComp.soundInstanceHandle);
				_soundComp.soundInstanceHandle = {};
			}
		}
	);

	// 被弾音も同じように返却する
	a_world.ReleaseTask<HitSoundComponent>(
		Engine::ECS::ESystemType::Release,
		"HitSoundFreeSystem",
		[]
		(
			Engine::ECS::ArchetypeChunk* a_pChunk,
			uint32_t a_count,
			const Engine::ECS::SystemContext& a_ctx,
			ReleaseTag* a_releaseTag,
			HitSoundComponent* a_hitSoundArray
			)
		{
			auto* _pAudioManager = a_ctx.pServices->pAudioManager;
			if (!_pAudioManager) return;

			for (size_t _i = 0; _i < a_count; ++_i)
			{
				HitSoundComponent& _hitSoundComp = a_hitSoundArray[_i];

				_pAudioManager->ReleaseSoundInstance(_hitSoundComp.soundInstanceHandle);
				_hitSoundComp.soundInstanceHandle = {};
			}
		}
	);

	// エフェクトのサウンドパーツぶんの声も返す。
	// 返さないとプールに鳴りっぱなしの声が残り、
	// 出しては消える単発エフェクトのぶんだけ溜まっていく
	a_world.ReleaseTask<EffectAssetComponent>(
		Engine::ECS::ESystemType::Release,
		"EffectSoundFreeSystem",
		[]
		(
			Engine::ECS::ArchetypeChunk* a_pChunk,
			uint32_t a_count,
			const Engine::ECS::SystemContext& a_ctx,
			ReleaseTag* a_releaseTag,
			EffectAssetComponent* a_effectArray
			)
		{
			auto* _pAudioManager = a_ctx.pServices->pAudioManager;
			if (!_pAudioManager) return;

			for (size_t _i = 0; _i < a_count; ++_i)
			{
				a_effectArray[_i].instance.ReleaseSounds(*_pAudioManager);
			}
		}
	);

	// オーディオビヘイビアはフェーズぶんのインスタンスを持っているので、
	// まとめて返却する(鳴っていても止まる)
	a_world.ReleaseTask<AudioBehaviorComponent>(
		Engine::ECS::ESystemType::Release,
		"AudioBehaviorFreeSystem",
		[]
		(
			Engine::ECS::ArchetypeChunk* a_pChunk,
			uint32_t a_count,
			const Engine::ECS::SystemContext& a_ctx,
			ReleaseTag* a_releaseTag,
			AudioBehaviorComponent* a_behaviorArray
			)
		{
			auto* _pAudioManager = a_ctx.pServices->pAudioManager;
			if (!_pAudioManager) return;

			for (size_t _i = 0; _i < a_count; ++_i)
			{
				a_behaviorArray[_i].instance.Release(*_pAudioManager);
			}
		}
	);
}
