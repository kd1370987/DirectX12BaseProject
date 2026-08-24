#include "CloseCombatIntentSystem.h"

#include "Engine/ECS/World/World.h"

#include "../../../../Components/Character/CloseCombatComponent.h"
#include "../../../../Components/Character/PatrolComponent.h"
#include "../../../../Components/Character/TargetEntityComponent.h"
#include "../../../../Components/Intent/ActionIntentComponent.h"
#include "../../../../Components/Intent/MoveIntentComponent.h"
#include "../../../../Components/Transform/LocalTransformComponent.h"
#include "../../../../Components/Transform/WorldMatrixComponent.h"

//==============================================================================
// CloseCombatIntentSystem
//
// 近距離型の敵(CloseCombatComponent 保持者)の行動を決める。
// 「足を止めて撃つ」と「撃たずに動き直す」を fireTime / moveTime で交互に回す。
//
// ・働くのは攻撃圏(isInAttackRange)の内側だけ。
//   見つけて詰めている途中と徘徊は EnemyMoveIntentSystem がそのまま持つので、
//   ここは MoveIntent に触らず素通りする。攻撃圏に入ったときだけ上書きする。
//
// ・EnemyMoveIntentSystem と同じ MoveIntentComponent を書く。
//   書き手同士の順序は依存では決まらない(RAW だけが辺になる)ので、
//   あちらが書く PatrolComponent をこのシステムが読むことで後ろへ回している。
//   PatrolComponent を読むのをやめると、上書きしたはずの移動入力が
//   あちらの追従の値に戻され、足を止めなくなる。
//
// ・発射入力(ActionIntentComponent)は EnemyShootIntentSystem も書く。
//   こちらは読む値が無く順序を作れないので、あちら側で
//   Exclude<CloseCombatComponent> して書き手を1つに絞ってある。
//
// ・動く相の向きは「横へ回り込む成分」と「間合いを直す成分」の合成。
//   真横だけだと間合いがずれたまま戻らず、詰め/離しだけだと
//   ただの前後移動になって近距離戦らしくならない。
//==============================================================================
void CloseCombatIntentSystem::Init(Engine::ECS::World& a_world)
{
	a_world.ActiveTask<const TargetEntityComponent, const PatrolComponent, const LocalTransformComponent,
		CloseCombatComponent, MoveIntentComponent, ActionIntentComponent>(
		Engine::ECS::ESystemType::PreUpdate,
		"CloseCombatIntentSystem",
		[](
			Engine::ECS::ArchetypeChunk*      a_pChunk,
			uint32_t                          a_count,
			const Engine::ECS::SystemContext& a_ctx,
			ActiveTag*                        a_tags,
			const TargetEntityComponent*      a_targetArray,
			const PatrolComponent*            a_patrolArray,
			const LocalTransformComponent*    a_trsArray,
			CloseCombatComponent*             a_combatArray,
			MoveIntentComponent*              a_moveArray,
			ActionIntentComponent*            a_actionArray
		)
		{
			for (size_t _i = 0; _i < a_count; ++_i)
			{
				const TargetEntityComponent&   _target = a_targetArray[_i];
				const LocalTransformComponent& _trs    = a_trsArray[_i];
				CloseCombatComponent&          _combat = a_combatArray[_i];
				MoveIntentComponent&           _move   = a_moveArray[_i];
				ActionIntentComponent&         _action = a_actionArray[_i];

				const bool _hasTarget = (_target.targetEntity != Engine::ECS::Limits::INVALID_ENTITY);

				//--------------------------------------------------------------
				// 攻撃圏の外 : 撃たないことだけ決めて、移動は追従に任せる
				//--------------------------------------------------------------
				if (!_hasTarget || !_target.isInAttackRange)
				{
					_action.isLeftWeaponShoot  = false;
					_action.isRightWeaponShoot = false;

					// 次に攻撃圏へ入ったとき、いきなり動く相から始まらないよう戻しておく
					_combat.isFirePhase = true;
					_combat.timer       = 0.0f;
					continue;
				}

				//--------------------------------------------------------------
				// 相の進行
				//--------------------------------------------------------------
				_combat.timer += a_ctx.dt;

				if (_combat.isFirePhase)
				{
					if (_combat.timer >= _combat.fireTime)
					{
						_combat.isFirePhase = false;
						_combat.timer       = 0.0f;

						// 回り込む向きは相に入るたびに選び直す。
						// 固定にすると同じ方向へ回り続けて、ぐるぐる回るだけになる
						_combat.sideSign = Math::Random::Sign();
					}
				}
				else if (_combat.timer >= _combat.moveTime)
				{
					_combat.isFirePhase = true;
					_combat.timer       = 0.0f;
				}

				//--------------------------------------------------------------
				// 撃つ相 : その場で足を止めて撃つ
				//--------------------------------------------------------------
				if (_combat.isFirePhase)
				{
					_move.value = { 0.0f, 0.0f, 0.0f };

					_action.isLeftWeaponShoot  = true;
					_action.isRightWeaponShoot = true;
					continue;
				}

				//--------------------------------------------------------------
				// 動く相 : 撃たずに位置を変える
				//--------------------------------------------------------------
				_action.isLeftWeaponShoot  = false;
				_action.isRightWeaponShoot = false;

				// ターゲットの位置が取れなければ動かない(向きを作れないため)
				const WorldMatrixComponent* _pTargetWorld = nullptr;
				if (a_ctx.pWorld->HasComponent<WorldMatrixComponent>(_target.targetEntity))
				{
					_pTargetWorld = a_ctx.pWorld->RefData<WorldMatrixComponent>(_target.targetEntity);
				}

				if (!_pTargetWorld)
				{
					_move.value = { 0.0f, 0.0f, 0.0f };
					continue;
				}

				const Math::Vector3 _targetPos = Math::Matrix(_pTargetWorld->worldMat).Translation();

				Math::Vector3 _to = _targetPos - Math::Vector3(_trs.pos);
				_to.y = 0.0f;	// 水平のみ。高さは重力に任せる

				const float _lenSq = _to.LengthSquared();
				if (_lenSq <= 1e-6f)
				{
					_move.value = { 0.0f, 0.0f, 0.0f };
					continue;
				}

				const Math::Vector3 _forward = _to / std::sqrt(_lenSq);

				// 左手系 +Z 前方なので、上 × 前 が右になる
				const Math::Vector3 _side =
					Math::Vector3(0.0f, 1.0f, 0.0f).Cross(_forward) * _combat.sideSign;

				// 間合いの直し : 遠ければ前(+)、近ければ後ろ(-)。
				// keepDistance ぶんずれたところで 1 になるよう正規化して、
				// 間合いが合っているときは横移動だけになるようにする
				float _radial = 0.0f;
				if (_combat.keepDistance > 1e-3f)
				{
					_radial = (_target.distance - _combat.keepDistance) / _combat.keepDistance;
					_radial = std::clamp(_radial, -1.0f, 1.0f);
				}

				Math::Vector3 _dir =
					_side * _combat.strafeRatio +
					_forward * (_radial * (1.0f - _combat.strafeRatio));

				const float _dirLenSq = _dir.LengthSquared();
				if (_dirLenSq <= 1e-6f)
				{
					_move.value = { 0.0f, 0.0f, 0.0f };
					continue;
				}

				_dir = _dir / std::sqrt(_dirLenSq);

				// 世界空間の水平方向 × スロットル(EnemyMovementSystem が速度へ直す)
				_move.value.x = _dir.x * _combat.moveThrottle;
				_move.value.y = 0.0f;
				_move.value.z = _dir.z * _combat.moveThrottle;
			}
		}
	);
}
