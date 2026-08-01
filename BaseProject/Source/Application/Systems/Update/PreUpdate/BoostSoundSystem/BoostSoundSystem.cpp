#include "BoostSoundSystem.h"

#include "Engine/ECS/World/World.h"

#include "../../../../Components/Character/Robot/AttachmentSlotsComponent.h"
#include "../../../../Components/Character/Robot/BoostComponent.h"
#include "../../../../Components/Resource/SoundComponent.h"
#include "../../../../../Engine/Audio/AudioManager.h"

//==========================================================================================
// BoostSoundSystem
//
// ブースト状態からサウンドを鳴らす。ThrusterEffectSystem のサウンド版で、
// 噴射エフェクトと同じスロット構成をそのまま使う。
//
// ・発進音 : 踏んだ瞬間に、スロットが指すブースター子エンティティの音を鳴らす(単発想定)
// ・継続音 : ブースト中、ブースターを付けている親エンティティの音を鳴らし続ける(ループ想定)
//
// 子エンティティと親自身の SoundComponent はこのクエリに含まれないため
// World::RefData で横断参照する(構造変更は行わないので反復中でも安全)。
//==========================================================================================
void BoostSoundSystem::Init(Engine::ECS::World& a_world)
{
	a_world.ActiveTask<const AttachmentSlotsComponent, const BoostComponent>(
		Engine::ECS::ESystemType::PreUpdate,
		"BoostSoundSystem",
		[]
		(
			Engine::ECS::ArchetypeChunk* a_pChunk,
			uint32_t a_count,
			const Engine::ECS::SystemContext& a_ctx,
			ActiveTag* a_tags,
			const AttachmentSlotsComponent* a_slotsArray,
			const BoostComponent* a_boostArray
			)
		{
			auto* _pAudioManager = a_ctx.pServices->pAudioManager;
			if (!_pAudioManager) return;

			// エンティティが持つサウンドインスタンスを引く
			// SoundComponent が無い/サウンド未設定なら nullptr
			auto _refSound = [&a_ctx](Engine::ECS::Entity a_e, const SoundComponent** a_ppComp)
				-> Engine::Resource::SoundInstance*
			{
				*a_ppComp = nullptr;
				if (a_e == Engine::ECS::Limits::INVALID_ENTITY) return nullptr;
				if (!a_ctx.pWorld->HasComponent<SoundComponent>(a_e)) return nullptr;

				auto* _pComp = a_ctx.pWorld->RefData<SoundComponent>(a_e);
				if (!_pComp) return nullptr;

				*a_ppComp = _pComp;
				return a_ctx.pServices->pAudioManager->RefInstance(_pComp->soundInstanceHandle);
			};

			// ブースターの発進音を頭から鳴らす
			auto _playBoosterSound = [&_refSound](Engine::ECS::Entity a_e)
			{
				const SoundComponent* _pComp = nullptr;
				auto* _pInstance = _refSound(a_e, &_pComp);
				if (!_pInstance) return;

				// Play は内部で Stop してから鳴らすので、連打しても頭から鳴り直す
				_pInstance->Play(_pComp->isLoop);
			};

			for (size_t _i = 0; _i < a_count; ++_i)
			{
				const AttachmentSlotsComponent& _slots = a_slotsArray[_i];
				const BoostComponent& _boost = a_boostArray[_i];

				// 実際に推力が出る条件。
				// 燃料切れで飛べないときに音だけ鳴らないよう、
				// RobotBoostSystem / ThrusterEffectSystem と同じ判定にしている
				const bool _hasFuel  = _boost.currentFuel > _boost.boostFuel;
				const bool _boosting = _boost.isBoostIntent && _hasFuel;
				const bool _justBoosted = _boost.isBoostTriger && _hasFuel;

				// ---- 発進音 : ブースター側で鳴らす ----
				if (_justBoosted)
				{
					_playBoosterSound(_slots.rightShoulderBoost.id);
					_playBoosterSound(_slots.leftShoulderBoost.id);
					_playBoosterSound(_slots.rightLegBoost.id);
					_playBoosterSound(_slots.leftLegBoost.id);
				}

				// ---- 継続音 : ブースターを付けている親側で鳴らす ----
				const SoundComponent* _pOwnerSound = nullptr;
				auto* _pOwnerInstance = _refSound(a_pChunk->entityData[_i], &_pOwnerSound);
				if (!_pOwnerInstance) continue;

				if (_boosting)
				{
					// ループ設定なら鳴りっぱなしになるので、
					// 止まっているときだけ鳴らし直せば二重再生にならない
					if (!_pOwnerInstance->IsPlay())
					{
						_pOwnerInstance->Play(_pOwnerSound->isLoop);
					}
				}
				else if (_pOwnerInstance->IsPlay())
				{
					_pOwnerInstance->Stop();
				}
			}
		}
	);
}
