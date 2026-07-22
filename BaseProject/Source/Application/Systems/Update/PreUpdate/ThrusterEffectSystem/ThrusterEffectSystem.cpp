#include "ThrusterEffectSystem.h"

#include "Engine/ECS/World/World.h"

#include "../../../../Components/Charactor/Robot/AttachmentSlotsComponent.h"
#include "../../../../Components/Charactor/Robot/BoostComponent.h"
#include "../../../../Components/Intent/MoveIntentComponent.h"
#include "../../../../Components/Force/VelocityComponent.h"
#include "../../../../Components/Resource/ParticlesComponent.h"

//==========================================================================================
// ThrusterEffectSystem
//
// プレイヤー(AttachmentSlotsComponent 保持者)の移動状態から、
// スロットが指すブースター子エンティティの噴射 ON/OFF を決める。
// 子エンティティはこのクエリに含まれないため World::RefData で横断参照する
// (構造変更は行わないので反復中でも安全)。
//==========================================================================================
void ThrusterEffectSystem::Init(Engine::ECS::World& a_world)
{
	a_world.ActiveTask<const AttachmentSlotsComponent, const MoveIntentComponent, const VelocityComponent, const BoostComponent>(
		Engine::ECS::ESystemType::PreUpdate,
		"ThrusterEffectSystem",
		[&a_world]
		(
			Engine::ECS::ArchetypeChunk* a_pChunk,
			uint32_t a_count,
			float a_dt,
			ActiveTag* a_tags,
			const AttachmentSlotsComponent* a_slotsArray,
			const MoveIntentComponent* a_moveArray,
			const VelocityComponent* a_velocityArray,
			const BoostComponent* a_boostArray
			)
		{
			// 微小な速度ノイズで点火しないための閾値
			constexpr float kMoveEps = 0.1f;	// 水平移動とみなす速さ
			constexpr float kRiseEps = 0.1f;	// 上昇とみなす速度

			// ブースター子の噴射 ON/OFF を設定
			auto _setBoosterPlay = [&a_world](Engine::ECS::Entity a_e, bool a_on)
			{
				if (a_e == Engine::ECS::Limits::INVALID_ENTITY) return;
				if (!a_world.HasComponent<ParticlesComponent>(a_e)) return;
				if (auto* _p = a_world.RefData<ParticlesComponent>(a_e)) _p->isPlay = a_on;
			};

			for (size_t _i = 0; _i < a_count; ++_i)
			{
				const AttachmentSlotsComponent& _slots = a_slotsArray[_i];
				const MoveIntentComponent& _move = a_moveArray[_i];
				const VelocityComponent& _velocity = a_velocityArray[_i];
				const BoostComponent& _boost = a_boostArray[_i];

				// ---- 移動状態の判定 ----

				// 入力で即応させつつ、実速度でも判定する
				bool _inputMoving =
					(_move.value.x != 0.0f) || (_move.value.z != 0.0f);

				float _hSpeedSq =
					_velocity.value.x * _velocity.value.x +
					_velocity.value.z * _velocity.value.z;

				bool _moving = _inputMoving || (_hSpeedSq > kMoveEps * kMoveEps);	// 水平移動
				bool _rising = _velocity.value.y > kRiseEps;						// 上昇(ジャンプ/上昇ブースト)

				// ブースト中か : 入力が入っていて、かつ燃料が使用量を上回っている
				// (RobotBoostSystem の推力適用条件に合わせている)
				bool _boosting = _boost.isBoostIntent && (_boost.currentFuel > _boost.boostFuel);

				// ---- スラスター2系統の点火判定 ----

				// 脚 : 通常移動・上昇のメイン推進
				bool _legOn = _moving || _rising || _boosting;

				// 肩 : ブースト時のアフターバーナー
				bool _shoulderOn = _boosting;

				// ---- ブースタースロットへ配信 ----
				_setBoosterPlay(_slots.rightLegBoost.id,      _legOn);
				_setBoosterPlay(_slots.leftLegBoost.id,       _legOn);
				_setBoosterPlay(_slots.rightShoulderBoost.id, _shoulderOn);
				_setBoosterPlay(_slots.leftShoulderBoost.id,  _shoulderOn);
			}
		}
	);
}
