#include "BoosterEffectSystem.h"

#include "Engine/ECS/World/World.h"

#include "Application/Components/Character/Robot/BoosterEffectComponent.h"
#include "Application/Components/Effect/EffectAssetComponent.h"

//==========================================================================================
// BoosterEffectSystem
//
// ブースターの「置き方」と「吹かした瞬間の膨らみ」を、噴射エフェクトへ渡す。
//
// ・エフェクトアセットは GUID 単位で共有されるので、取り付け位置と向きの個体差は
//   アセットには書けない。BoosterEffectComponent が持っているものを、
//   毎フレーム EffectAssetComponent の上書き欄へ写して渡す。
//
// ・膨らみは点火(isPlay の立ち上がり)で burstScale まで跳ね上げ、
//   burstTime かけて baseScale へ戻す。
//   点火/消火を決めるのは ThrusterEffectSystem(PreUpdate)なので、
//   ここは Update 帯に置いてそのフレームの isPlay を見る。
//   実際に出すのは EffectDrawSystem(Draw)なので、書いた値はそのフレームに間に合う。
//
// ・戻し方を「毎フレーム一定割合で減らす」ではなく残り時間の線形にしているのは、
//   burstTime にそのまま「何秒で戻るか」が出ていた方が調整しやすいため。
//==========================================================================================
void BoosterEffectSystem::Init(Engine::ECS::World& a_world)
{
	a_world.ActiveTask<BoosterEffectComponent, EffectAssetComponent>(
		Engine::ECS::ESystemType::Update,
		"BoosterEffectSystem",
		[](
			Engine::ECS::ArchetypeChunk*      a_pChunk,
			uint32_t                          a_count,
			const Engine::ECS::SystemContext& a_ctx,
			ActiveTag*                        a_tags,
			BoosterEffectComponent*           a_boosterArray,
			EffectAssetComponent*             a_effectArray
		)
		{
			for (size_t _i = 0; _i < a_count; ++_i)
			{
				BoosterEffectComponent& _booster = a_boosterArray[_i];
				EffectAssetComponent&   _effect  = a_effectArray[_i];

				//--------------------------------------------------------------
				// 点火の立ち上がりで膨らませる
				//--------------------------------------------------------------
				const bool _isPlaying = _effect.isPlay;

				if (_isPlaying && !_booster.wasPlaying)
				{
					_booster.burstTimer = _booster.burstTime;
				}
				_booster.wasPlaying = _isPlaying;

				// 時間を進める。消火中も戻しきってしまってよい
				// (次に点火したときはどうせ入れ直すため)
				if (_booster.burstTimer > 0.0f)
				{
					_booster.burstTimer = (std::max)(0.0f, _booster.burstTimer - a_ctx.dt);
				}

				//--------------------------------------------------------------
				// 大きさ : burstScale から baseScale へ戻していく
				//--------------------------------------------------------------
				float _t = 0.0f;	// 1 = 点火した瞬間 / 0 = 戻りきった
				if (_booster.burstTime > 0.0f)
				{
					_t = std::clamp(_booster.burstTimer / _booster.burstTime, 0.0f, 1.0f);
				}

				_effect.effectScale =
					_booster.baseScale + (_booster.burstScale - _booster.baseScale) * _t;

				//--------------------------------------------------------------
				// 置き方 : 噴射口の位置と向きを渡す
				//--------------------------------------------------------------
				_effect.isOverrideTransform = true;
				_effect.overridePosOffset   = _booster.posOffset;

				// 向きは正規化して渡す。
				// 手で入れた値は長さがまちまちなので、ここで揃えておかないと
				// 受け側(EffectDrawSystem)で行列を掛けたときに長さが効いてしまう。
				// 0 ベクトルのときは触らない(既定の向きのまま出す)
				Math::Vector3 _dir = _booster.emitDir;
				if (_dir.LengthSquared() > 1e-8f)
				{
					_dir.Normalize();
					_effect.overrideEmitDir = _dir;
				}
			}
		}
	);
}
