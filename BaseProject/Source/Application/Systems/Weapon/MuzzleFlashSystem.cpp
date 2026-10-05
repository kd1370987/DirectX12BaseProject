#include "MuzzleFlashSystem.h"

#include "Application/ECS/World/APPWorld.h"
#include "Engine/Resource/Manager/ResourceManager/ResourceManager.h"

#include "Application/Components/Weapon/GunStateComponent.h"
#include "Application/Components/Weapon/GunFireComponent.h"
#include "Application/Components/Transform/WorldMatrixComponent.h"
#include "Application/Components/Effect/EffectAssetComponent.h"
#include "Application/Components/Effect/EffectRuntimeComponent.h"
#include "Engine/Effect/EffectPlayer.h"
#include "Application/Components/Effect/EffectPlayRequestComponent.h"
#include "Application/Components/Effect/EffectOverrideComponent.h"

//==========================================================================================
// MuzzleFlashSystem
//
// 撃った銃(GunFireComponent::isFired)の銃口へ、銃自身が持っている再生枠
// (EffectAssetComponent と、その実行中の値)を頭から再生し直す。
//
// ・エフェクト用のエンティティは出さない。出すとそのエンティティは
//   エフェクトが終わるまでワールドに残るので、移動しながら撃つと
//   撃った位置に取り残されて尾を引いて見える。(枠を持たせるのは GunStateStartSystem)
// ・置き方は毎発入れ直す。銃口ノードは変わらなくても、狙いの向きは撃つたびに変わるため。
// ・弾が出なかったフレーム(プレハブ未設定など)では光らない。
//   GunProjectileSpawnSystem が弾を出せなかったときに isFired を下ろすので、その後に読む。
// ・エフェクトの Play を直接呼ぶのでメインスレッドで回す。
//==========================================================================================
void MuzzleFlashSystem::Init(App::ECS::APPWorld& a_world)
{
	a_world.ActiveTask<const GunStateComponent, const GunFireComponent, const WorldMatrixComponent,
		const EffectAssetComponent, EffectRuntimeComponent, EffectPlayRequestComponent, EffectOverrideComponent>(
		Engine::ECS::ESystemType::Update,
		"MuzzleFlashSystem",
		[]
		(
			Engine::ECS::Chunk*,
			uint32_t a_count,
			const Engine::ECS::SystemContext& a_ctx,
			ActiveTag*,
			const GunStateComponent* a_gunArray,
			const GunFireComponent* a_fireArray,
			const WorldMatrixComponent* a_worldMatArray,
			const EffectAssetComponent*,
			EffectRuntimeComponent* a_runtimeArray,
			EffectPlayRequestComponent* a_requestArray,
			EffectOverrideComponent* a_overrideArray
		)
		{
			for (uint32_t _i = 0; _i < a_count; ++_i)
			{
				const GunFireComponent& _fireComp = a_fireArray[_i];
				if (!_fireComp.isFired) continue;

				const GunStateComponent& _gun = a_gunArray[_i];
				if (_gun.muzzleEffectGUID == Engine::DefaultGUID) continue;

				EffectRuntimeComponent& _runtime = a_runtimeArray[_i];
				EffectOverrideComponent& _override = a_overrideArray[_i];
				auto* _pMuzzleEffect = a_ctx.pServices->pResourceManager->Ref(_runtime.effectHandle);
				if (!_pMuzzleEffect) continue;

				// 位置も向きも銃のローカルへ直して渡す。
				// EffectDrawSystem がこの銃のワールド行列を掛けるので、
				// ワールドのまま渡すと二重に掛かる
				const Math::Matrix _gunInvMat = Math::Matrix(a_worldMatArray[_i].worldMat).Invert();

				_override.effectScale = _gun.muzzleEffectScale;
				_override.isOverrideTransform = true;
				_override.overridePosOffset = _fireComp.muzzleLocalPos;
				_override.overrideEmitDir = Math::Vector3::TransformNormal(_fireComp.shootDir, _gunInvMat);

				// 頭から出し直す(Play は中で Reset を呼ぶ)。
				//
				// isPlay は一度立てたら下ろさない。EffectUpdateSystem は
				// isPlay の立ち下がりで Stop するので、下ろすと消えてしまう。
				// また立ち上がりの Play は「まだ再生していない」ときしか走らず、
				// 連射の上書きには使えないので、ここで直接 Play を呼ぶ。
				// 出し切った後はどのパーツも出す時間帯から外れるだけなので、
				// 再生中のまま置いておいても何も出ない
				a_requestArray[_i].isPlay = true;
				Engine::Effect::EffectPlayer::Play(*_pMuzzleEffect, _runtime.instance);
			}
		}
	)
	// 順序 : エフェクト(EffectAsset)の書き手同士の並び
	.After("BoosterEffectSystem");
}
