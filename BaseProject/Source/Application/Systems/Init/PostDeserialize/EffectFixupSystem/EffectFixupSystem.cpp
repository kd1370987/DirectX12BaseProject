#include "EffectFixupSystem.h"

#include "Engine/ECS/World/World.h"

#include "../../../../Components/Tag/SystemPhaseTag/PostDeserializeTag.h"
#include "../../../../Components/Effect/EffectAssetComponent.h"
#include "../../../../Components/Character/DeathEffectComponent.h"

//==========================================================================================
// EffectFixupSystem
//
// GUID しか保存されていないエフェクトから、アセットのハンドルを解決し直す。
// あわせて、進行状態(保存されないランタイム値)を作り直した状態に戻す。
//
// 死亡エフェクト(DeathEffectComponent)も同じ EffectAsset を指すので、ここで一緒に解決する。
// あちらは「死んだときに出すもの」なので進行状態は持たず、ハンドルを引くだけ。
// 死んだ瞬間に読み込みが走らないよう、生成時に解決しておく。
//==========================================================================================
void EffectFixupSystem::Init(Engine::ECS::World& a_world)
{
	//--------------------------------------------------------------------------
	// 再生するエフェクト : ハンドルと進行状態
	//--------------------------------------------------------------------------
	a_world.PostDeserializeTask<EffectAssetComponent>(
		Engine::ECS::ESystemType::PostDeserialize,
		"EffectFixupSystem",
		[]
		(
			Engine::ECS::ArchetypeChunk* a_pChunk,
			uint32_t a_count,
			const Engine::ECS::SystemContext& a_ctx,
			PostDeserializeTag* a_tag,
			EffectAssetComponent* a_effectArray
			)
		{
			auto* _pResourceManager = a_ctx.pServices->pResourceManager;
			if (!_pResourceManager) return;

			for (size_t _i = 0; _i < a_count; ++_i)
			{
				EffectAssetComponent& _effectComp = a_effectArray[_i];

				// 進行状態はランタイム値なので、作り直しでリセットしておく。
				// これをしないと、差し替え前の再生位置から続きが出てしまう
				_effectComp.instance = {};

				// 出っぱなしの指定なら、ここで再生状態にしておく。
				// isPlay は保存されないので、誰かが立てないと何も出ない
				_effectComp.isPlay = _effectComp.playOnStart;

				if (_effectComp.effectGUID == Engine::DefaultGUID)
				{
					_effectComp.effectHandle = {};
					continue;
				}

				_effectComp.effectHandle =
					_pResourceManager->LoadImmediate<Engine::Resource::EffectAsset>(_effectComp.effectGUID);
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
			Engine::ECS::ArchetypeChunk* a_pChunk,
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

				_deathComp.effectHandle =
					_pResourceManager->LoadImmediate<Engine::Resource::EffectAsset>(_deathComp.effectGUID);
			}
		}
	);
}
