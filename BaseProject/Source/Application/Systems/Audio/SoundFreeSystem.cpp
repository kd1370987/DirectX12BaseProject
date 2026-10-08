#include "SoundFreeSystem.h"

#include "Application/ECS/World/APPWorld.h"

#include "Application/Components/Effect/EffectRuntimeComponent.h"
#include "Engine/Audio/AudioManager.h"

namespace App::System
{
	void SoundFreeSystem::Init(App::ECS::APPWorld& a_world)
	{
		// エフェクトのサウンドパーツぶんの声を返す。
		// 返さないとプールに鳴りっぱなしの声が残り、
		// 出しては消える単発エフェクトのぶんだけ溜まっていく
		a_world.ReleaseTask<Component::EffectRuntimeComponent>(
			Engine::ECS::ESystemType::Release,
			"EffectSoundFreeSystem",
			[]
			(
				Engine::ECS::Chunk* /*a_pChunk*/,
				uint32_t a_count,
				const Engine::ECS::SystemContext& a_ctx,
				Component::ReleaseTag* /*a_releaseTag*/,
				Component::EffectRuntimeComponent* a_effectArray
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
	}
}
