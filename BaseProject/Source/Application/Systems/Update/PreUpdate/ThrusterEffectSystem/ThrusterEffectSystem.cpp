#include "ThrusterEffectSystem.h"

#include "Engine/ECS/World/World.h"

#include "../../../../Components/Character/Robot/AttachmentSlotsComponent.h"
#include "../../../../Components/Character/Robot/BoostComponent.h"
#include "../../../../Components/Intent/MoveIntentComponent.h"
#include "../../../../Components/Force/VelocityComponent.h"
#include "../../../../Components/Effect/EffectAssetComponent.h"
#include "../../../../Components/Character/Robot/BoosterEffectComponent.h"

//==========================================================================================
// ThrusterEffectSystem
//
// プレイヤー(AttachmentSlotsComponent 保持者)の移動状態から、
// スロットが指すブースター子エンティティの噴射 ON/OFF を決める。
// 子エンティティはこのクエリに含まれないため World::RefData で横断参照する
// (構造変更は行わないので反復中でも安全)。
//
// 噴射の中身は EffectAsset(パーティクル+メッシュ)が持ち、
// 取り付け位置や吹かしたときの膨らみは BoosterEffectComponent が持つので、
// ここが伝えるのは「噴いているか」と「ブーストダッシュ中か」の2つだけ。
//
// ダッシュ中かを別に配るのは、ジェットの見え方が2段あるため。
// 通常移動でも噴射は出るので、ON/OFF だけでは歩いている時と
// 一気に加速した時が同じ絵になってしまう。ダッシュ中はジェットを太らせ、
// 踏み込んだ瞬間にはスパークを出す。どちらも実際に動かすのは
// BoosterEffectSystem(Update)で、ここはその材料を渡すだけ。
//
// ダッシュ判定に入力の「押した瞬間」(isBoostTriger)を使わないのは、
// あれがプレイヤー入力でしか立たないため。ボスのように isBoostIntent だけ
// 立てて噴射に入る相手でも同じ演出が出るよう、BoostSoundSystem と同じく
// 「実際に推力が出ているか」で見る(立ち上がりは受け取った側が見る)。
//
// 以前はブースターに ParticlesComponent を直に付けていて、
// それ向けの分岐もここにあったが、ブースターは全て EffectAsset へ移したので消した。
//==========================================================================================
void ThrusterEffectSystem::Init(Engine::ECS::World& a_world)
{
	a_world.ActiveTask<const AttachmentSlotsComponent, const MoveIntentComponent, const VelocityComponent, const BoostComponent>(
		Engine::ECS::ESystemType::PreUpdate,
		"ThrusterEffectSystem",
		[]
		(
			Engine::ECS::ArchetypeChunk* a_pChunk,
			uint32_t a_count,
			const Engine::ECS::SystemContext& a_ctx,
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

			// ブースター子へ噴射の ON/OFF とダッシュ中かを配る。
			// RefData は持っていないコンポーネントでも非nullを返すので、
			// 必ず HasComponent で確かめてから引くこと。
			// BoosterEffectComponent を付けていないブースターもあり得るので、
			// 2つは別々に確かめる(付いていない側は黙って飛ばす)
			auto _driveBooster = [&a_ctx](Engine::ECS::Entity a_e, bool a_on, bool a_isBoosting)
			{
				if (a_e == Engine::ECS::Limits::INVALID_ENTITY) return;

				if (a_ctx.pWorld->HasComponent<EffectAssetComponent>(a_e))
				{
					if (auto* _pEffect = a_ctx.pWorld->RefData<EffectAssetComponent>(a_e))
					{
						_pEffect->isPlay = a_on;
					}
				}

				if (a_ctx.pWorld->HasComponent<BoosterEffectComponent>(a_e))
				{
					if (auto* _pBooster = a_ctx.pWorld->RefData<BoosterEffectComponent>(a_e))
					{
						_pBooster->isBoosting = a_isBoosting;
					}
				}
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

				bool _boostOn = _moving || _rising || _boosting;

				// ---- ブースタースロットへ配信 ----
				_driveBooster(_slots.rightLegBoost.id, _boostOn, _boosting);
				_driveBooster(_slots.leftLegBoost.id, _boostOn, _boosting);
				_driveBooster(_slots.rightShoulderBoost.id, _boostOn, _boosting);
				_driveBooster(_slots.leftShoulderBoost.id, _boostOn, _boosting);
			}
		}
	);
}
