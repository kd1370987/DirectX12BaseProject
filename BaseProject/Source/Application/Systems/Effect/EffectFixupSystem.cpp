#include "EffectFixupSystem.h"

#include "Application/ECS/World/APPWorld.h"

#include "Application/Components/Core/PhaseTag/PostDeserializeTag.h"
#include "Application/Components/Effect/EffectAssetComponent.h"
#include "Engine/Effect/EffectPlayer.h"
#include "Application/Components/Combat/DeathEffectComponent.h"
#include "Application/Components/Effect/BoosterEffectComponent.h"
#include "Application/Components/Weapon/GunStateComponent.h"

#include "../../Utility/EffectSpawnHelper.h"

//==========================================================================================
// EffectFixupSystem
//
// GUID しか保存されていないエフェクトから、アセットのハンドルを解決し直す。
// あわせて、進行状態(保存されないランタイム値)を作り直した状態に戻す。
//
// エフェクトにサウンドパーツが入っていれば、再生用の声もここで確保する。
// 鳴らす瞬間にインスタンスを作ると、その1フレームだけ音が遅れるため。
//
// 死亡エフェクト(DeathEffectComponent)・ブーストのスパーク(BoosterEffectComponent)・
// 銃のマズルフラッシュ(GunStateComponent)も同じ EffectAsset を指すので、ここで一緒に解決する。
// どれも「その瞬間が来たら出すもの」なので進行状態は持たず、ハンドルを引くだけ。
// 死んだ瞬間・踏み込んだ瞬間・撃った瞬間に読み込みが走らないよう、生成時に解決しておく。
//==========================================================================================
void EffectFixupSystem::Init(App::ECS::APPWorld& a_world)
{
	//--------------------------------------------------------------------------
	// 再生するエフェクト : ハンドルと進行状態
	//--------------------------------------------------------------------------
	a_world.PostDeserializeTask<const EffectAssetComponent, EffectRuntimeComponent, EffectPlayRequestComponent>(
		Engine::ECS::ESystemType::PostDeserialize,
		"EffectFixupSystem",
		[]
		(
			Engine::ECS::Chunk* a_pChunk,
			uint32_t a_count,
			const Engine::ECS::SystemContext& a_ctx,
			PostDeserializeTag* a_tag,
			const EffectAssetComponent* a_effectArray,
			EffectRuntimeComponent* a_runtimeArray,
			EffectPlayRequestComponent* a_requestArray
			)
		{
			auto* _pResourceManager = a_ctx.pServices->pResourceManager;
			if (!_pResourceManager) return;

			auto* _pAudioManager = a_ctx.pServices->pAudioManager;

			for (size_t _i = 0; _i < a_count; ++_i)
			{
				const EffectAssetComponent& _effectComp = a_effectArray[_i];
				EffectRuntimeComponent& _runtime = a_runtimeArray[_i];
				EffectPlayRequestComponent& _request = a_requestArray[_i];

				// 借りている声を先に返す。
				// 進行状態を丸ごと潰すと、ハンドルまで消えて返却先が分からなくなる
				if (_pAudioManager)
				{
					_runtime.instance.ReleaseSounds(*_pAudioManager);
				}

				// 進行状態はランタイム値なので、作り直しでリセットしておく。
				// これをしないと、差し替え前の再生位置から続きが出てしまう
				_runtime.instance = {};

				// 発生源の席も空にする(返さない)。
				// 入り直しのときは Release フックが返却を予約し終えている。
				// 新しく実体化したものは、プレハブや複製元の値が写っているだけで自分の席ではないので、
				// 返すと他人の席を手放してしまう。どちらにしても、ここでは忘れるだけでよい
				_runtime.emitterSlot = {};

				// 出っぱなしの指定なら、ここで再生状態にしておく。
				// isPlay は保存されないので、誰かが立てないと何も出ない
				_request.isPlay = _effectComp.playOnStart;

				if (_effectComp.effectGUID == Engine::DefaultGUID)
				{
					_runtime.effectHandle = {};
					continue;
				}

				_pResourceManager->AcquireImmediate(_runtime.effectHandle, _effectComp.effectGUID);
				App::Utility::WarmupEffectParticles(*a_ctx.pServices, _runtime.effectHandle);

				// 声は空の状態から始める(借りるのは鳴らす直前、返すのは鳴り終わったとき)。
				// 波形はアセットの解決(ResolveReferences)で読んであるので、鳴らす瞬間に読み込みは走らない
				if (_pAudioManager)
				{
					if (auto* _pEffect = _pResourceManager->Ref(_runtime.effectHandle))
					{
						Engine::Effect::EffectPlayer::PrepareSounds(*_pEffect, *_pAudioManager, _runtime.instance);
					}
				}
			}
		}
	);

	//--------------------------------------------------------------------------
	// 死亡エフェクト : ハンドルを解決するだけ
	//--------------------------------------------------------------------------
	a_world.PostDeserializeTask<DeathEffectComponent>(
		Engine::ECS::ESystemType::PostDeserialize,
		"DeathEffectFixupSystem",
		[]
		(
			Engine::ECS::Chunk* a_pChunk,
			uint32_t a_count,
			const Engine::ECS::SystemContext& a_ctx,
			PostDeserializeTag* a_tag,
			DeathEffectComponent* a_deathArray
			)
		{
			auto* _pResourceManager = a_ctx.pServices->pResourceManager;
			if (!_pResourceManager) return;

			for (size_t _i = 0; _i < a_count; ++_i)
			{
				DeathEffectComponent& _deathComp = a_deathArray[_i];

				if (_deathComp.effectGUID == Engine::DefaultGUID)
				{
					_deathComp.effectHandle = {};
					continue;
				}

				_pResourceManager->AcquireImmediate(_deathComp.effectHandle, _deathComp.effectGUID);
				App::Utility::WarmupEffectParticles(*a_ctx.pServices, _deathComp.effectHandle);
			}
		}
	);

	//--------------------------------------------------------------------------
	// ブーストのスパーク : ハンドルを解決するだけ
	//--------------------------------------------------------------------------
	a_world.PostDeserializeTask<BoosterEffectComponent>(
		Engine::ECS::ESystemType::PostDeserialize,
		"BoosterSparkFixupSystem",
		[]
		(
			Engine::ECS::Chunk* a_pChunk,
			uint32_t a_count,
			const Engine::ECS::SystemContext& a_ctx,
			PostDeserializeTag* a_tag,
			BoosterEffectComponent* a_boosterArray
			)
		{
			auto* _pResourceManager = a_ctx.pServices->pResourceManager;
			if (!_pResourceManager) return;

			for (size_t _i = 0; _i < a_count; ++_i)
			{
				BoosterEffectComponent& _boosterComp = a_boosterArray[_i];

				// 見え方の進み具合はランタイム値なので、作り直しで戻しておく。
				// 残っていると、差し替えた直後だけ太いまま出てしまう
				_boosterComp.burstTimer = 0.0f;
				_boosterComp.wasPlaying = false;
				_boosterComp.isBoosting = false;
				_boosterComp.wasBoosting = false;
				_boosterComp.boostBlend = 0.0f;

				if (_boosterComp.sparkEffectGUID == Engine::DefaultGUID)
				{
					_boosterComp.sparkHandle = {};
					continue;
				}

				_pResourceManager->AcquireImmediate(_boosterComp.sparkHandle, _boosterComp.sparkEffectGUID);
				App::Utility::WarmupEffectParticles(*a_ctx.pServices, _boosterComp.sparkHandle);
			}
		}
	);

	//--------------------------------------------------------------------------
	// 銃のマズルフラッシュ : ハンドルを解決するだけ
	//--------------------------------------------------------------------------
	a_world.PostDeserializeTask<GunStateComponent>(
		Engine::ECS::ESystemType::PostDeserialize,
		"MuzzleEffectFixupSystem",
		[]
		(
			Engine::ECS::Chunk* a_pChunk,
			uint32_t a_count,
			const Engine::ECS::SystemContext& a_ctx,
			PostDeserializeTag* a_tag,
			GunStateComponent* a_gunArray
			)
		{
			auto* _pResourceManager = a_ctx.pServices->pResourceManager;
			if (!_pResourceManager) return;

			for (size_t _i = 0; _i < a_count; ++_i)
			{
				GunStateComponent& _gunComp = a_gunArray[_i];

				if (_gunComp.muzzleEffectGUID == Engine::DefaultGUID)
				{
					_gunComp.muzzleEffectHandle = {};
					continue;
				}

				_pResourceManager->AcquireImmediate(_gunComp.muzzleEffectHandle, _gunComp.muzzleEffectGUID);
				App::Utility::WarmupEffectParticles(*a_ctx.pServices, _gunComp.muzzleEffectHandle);
			}
		}
	);
}
