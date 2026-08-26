#include "HitSoundSystem.h"

#include "Application/ECS/World/World.h"

#include "Application/Components/Resource/HitSoundComponent.h"
#include "Application/InstanceResource/HitEventResource.h"

//==============================================================================
// HitSoundSystem
//
// そのフレームに攻撃を受けたエンティティから、被弾音を鳴らす。
// 弾がプレイヤーに当たったときにプレイヤー側で音を出すのがこれ。
//
// ・ヒットは HitEventResource(ワールドに1つ)を見る。
//   弾は当たった次の瞬間に自分を消すので、弾側のコンポーネントを見に行くと
//   間に合わない。逆に「誰が受けたか(victim)」はワールドに残るのでこちらを読む。
//   CollisionEvent(エンティティ単位・最新1件)ではなくこちらを読む理由も同じで、
//   受けた側の CollisionEvent は他のヒットで上書きされうる。
// ・鳴らすのは受けた側(victim)のインスタンス。attacker(弾)は既に居ない前提。
// ・PostUpdate 帯に置く。ヒットを積むのは Physics 帯の HitDetectSystem、
//   消すのは次フレーム PreUpdate の HitEventClearSystem なので、この間で読む。
// ・SoundInstance は1コンポーネントに1つしか無く、Play は頭出しの鳴らし直しになる。
//   連射を受けている間に音が潰れ続けないよう minInterval で間引く。
//==============================================================================
void HitSoundSystem::Init(App::ECS::World& a_world)
{
	a_world.ActiveTask<HitSoundComponent>(
		Engine::ECS::ESystemType::PostUpdate,
		"HitSoundSystem",
		[](
			Engine::ECS::ArchetypeChunk*      a_pChunk,
			uint32_t                          a_count,
			const Engine::ECS::SystemContext& a_ctx,
			ActiveTag*                        a_tags,
			HitSoundComponent*                a_hitSoundArray
		)
		{
			auto* _pAudioManager = a_ctx.pServices->pAudioManager;
			if (!_pAudioManager) return;

			if (!a_ctx.pWorld->HasResource<HitEventResource>()) return;
			const HitEventResource& _hitEvents = a_ctx.pWorld->GetResource<HitEventResource>();

			for (size_t _i = 0; _i < a_count; ++_i)
			{
				HitSoundComponent& _hitSound = a_hitSoundArray[_i];

				// 前回鳴らしてからの間隔を進める
				if (_hitSound.coolTime > 0.0f)
				{
					_hitSound.coolTime -= a_ctx.dt;
					if (_hitSound.coolTime < 0.0f) _hitSound.coolTime = 0.0f;
				}

				// 音が設定されていないなら何もしない
				if (_hitSound.soundGUID == Engine::DefaultGUID) continue;

				// 自分が受け手になっているヒットが今フレームにあるか
				// (現状ここへ流れてくるのは弾のヒットのみ。近接などが増えても
				//  産む側が victim を入れるので、そのまま鳴る)
				if (!_hitEvents.HasVictim(a_pChunk->entityData[_i])) continue;

				// 間引き中は鳴らさない
				if (_hitSound.coolTime > 0.0f) continue;

				auto* _pInstance = _pAudioManager->RefInstance(_hitSound.soundInstanceHandle);
				if (!_pInstance) continue;

				// Play は内部で Stop してから鳴らすので、被弾のたびに頭から鳴り直す
				_pInstance->Play(false);

				_hitSound.coolTime = _hitSound.minInterval;
			}
		}
	);
}
