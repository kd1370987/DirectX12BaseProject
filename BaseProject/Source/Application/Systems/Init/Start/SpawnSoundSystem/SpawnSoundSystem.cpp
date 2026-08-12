#include "SpawnSoundSystem.h"

#include "Engine/ECS/World/World.h"
#include "Engine/Audio/AudioManager.h"

#include "Application/Components/Resource/SoundComponent.h"

//==============================================================================
// SpawnSoundSystem
//
// isPlayOnSpawn が立っている SoundComponent を、湧いた瞬間に鳴らす。
// 爆発などのエフェクト用で、これを付けておけばエフェクトを出した側は
// 音のことを知らなくてよく、プレハブ単体で演出が完結する。
//
// ・Start フェーズに置く。エンティティは実体化した BeginFrame で
//   PostDeserialize → Awake → Start と一気に通ってから Active になるので、
//   ここが「湧いた瞬間」であり、かつ SoundFixupSystem(PostDeserialize)が
//   再生用インスタンスを発行し終えた後になる。
// ・Start は実体化のときに一度しか通らないので、鳴らしたかを覚える必要がない。
//   ただしエディターでのリフレッシュやコンポーネント追加でも
//   PostDeserialize からやり直しになるため、そのときは鳴り直す。
// ・isLoop も見る。単発の効果音は false、湧いてから鳴りっぱなしにしたい音は true。
//==============================================================================
void SpawnSoundSystem::Init(Engine::ECS::World& a_world)
{
	a_world.StartTask<SoundComponent>(
		Engine::ECS::ESystemType::Start,
		"SpawnSoundSystem",
		[]
		(
			Engine::ECS::ArchetypeChunk*      a_pChunk,
			uint32_t                          a_count,
			const Engine::ECS::SystemContext& a_ctx,
			StartTag*                         a_startTag,
			SoundComponent*                   a_soundArray
			)
		{
			auto* _pAudioManager = a_ctx.pServices->pAudioManager;
			if (!_pAudioManager) return;

			for (size_t _i = 0; _i < a_count; ++_i)
			{
				const SoundComponent& _sound = a_soundArray[_i];

				if (!_sound.isPlayOnSpawn) continue;

				// サウンド未設定 / 発行に失敗しているものは静かに飛ばす
				auto* _pInstance = _pAudioManager->RefInstance(_sound.soundInstanceHandle);
				if (!_pInstance) continue;

				// 音量は SoundFixupSystem が発行時に入れているので、ここでは鳴らすだけ
				// (2D の Play は音量を戻さない)
				_pInstance->Play(_sound.isLoop);
			}
		}
	);
}
