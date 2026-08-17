#include "HomingSystem.h"

#include "Engine/ECS/World/World.h"

#include "../../../../Components/Character/Weapon/Projectile/HomingComponent.h"
#include "../../../../Components/Force/VelocityComponent.h"
#include "../../../../Components/Transform/LocalTransformComponent.h"
#include "../../../../Components/Transform/WorldMatrixComponent.h"

//==============================================================================
// HomingSystem
//
// HomingComponent を持つ投射物(誘導弾)を、targetEntity へ向けて曲げる。
// targetEntity は発射した瞬間に GunShootSystem が埋める(敵なら索敵で見つけた
// エンティティ、プレイヤーなら狙点が当たっている相手)。
//
// ・速度ベクトルの「向き」だけを回し、大きさ(弾速)はそのまま保つ。
//   進む処理自体は PositionIntegrationSystem(Physics 帯)が行うので、
//   ここは速度を書き換えるだけでよい。
// ・PreUpdate 帯に置く理由
//     「今フレームどっちへ進むか」を決めるだけなので、敵の行動決定
//     (EnemyMoveIntentSystem)と同じ帯が素直。位置は前フレームの積分結果を読む。
//     加えて、Update 帯に置くとシステムのソートが循環する。
//     LockOnRotationSystem が Velocity を読んで LocalTransform を書き、
//     こちらは LocalTransform を読んで Velocity を書くため、
//     依存グラフ(コンポーネント単位・ワールド全体)が互いを指してしまう。
//     弾とプレイヤーはアーキタイプが別で実際には競合しないが、
//     グラフはそこまで見ないので帯を分けて避ける。
// ・turnSpeed は 1 秒あたりの旋回角(ラジアン/秒)。この角度以上は曲がれないので、
//   小さいほど大回り、大きいほど食いつく。0 以下なら誘導しない(直進)。
// ・searchRange は「その距離までなら追う」上限。離れすぎている間は曲げずに直進し、
//   また範囲に入れば追い直す(ターゲットは捨てないので再追尾できる)。
//   0 以下なら距離制限なし。
// ・ターゲットが消滅した場合、シグネチャが空になって HasComponent が false になる。
//   その時は targetEntity を無効値へ戻し、以降は直進させる。
// ・自分の位置は LocalTransform を見る。弾は親を持たない(ワールド=ローカル)し、
//   WorldMatrix は前フレームの PostUpdate の値なので、生成直後は入っていない。
//==============================================================================
void HomingSystem::Init(Engine::ECS::World& a_world)
{
	a_world.ActiveTask<HomingComponent, VelocityComponent, const LocalTransformComponent>(
		Engine::ECS::ESystemType::PreUpdate,
		"HomingSystem",
		[](
			Engine::ECS::ArchetypeChunk*      a_pChunk,
			uint32_t                          a_count,
			const Engine::ECS::SystemContext& a_ctx,
			ActiveTag*                        a_tags,
			HomingComponent*                  a_homingArray,
			VelocityComponent*                a_velArray,
			const LocalTransformComponent*    a_trsArray
		)
		{
			for (size_t _i = 0; _i < a_count; ++_i)
			{
				HomingComponent&               _homing = a_homingArray[_i];
				VelocityComponent&             _vel    = a_velArray[_i];
				const LocalTransformComponent& _trs    = a_trsArray[_i];

				// 曲がれないなら何もしない(直進)
				if (_homing.turnSpeed <= 0.0f) continue;

				//------------------------------------------------------
				// ターゲットの解決
				//   消滅していれば無効値へ戻して以降は直進させる
				//------------------------------------------------------
				if (_homing.targetEntity == Engine::ECS::Limits::INVALID_ENTITY) continue;

				if (!a_ctx.pWorld->HasComponent<WorldMatrixComponent>(_homing.targetEntity))
				{
					_homing.targetEntity = Engine::ECS::Limits::INVALID_ENTITY;
					continue;
				}

				const auto* _pTargetWorld =
					a_ctx.pWorld->RefData<WorldMatrixComponent>(_homing.targetEntity);
				if (!_pTargetWorld)
				{
					_homing.targetEntity = Engine::ECS::Limits::INVALID_ENTITY;
					continue;
				}

				Math::Vector3 _targetPos = Math::Matrix(_pTargetWorld->worldMat).Translation();

				//------------------------------------------------------
				// 現在の進行方向(速度が無いと向きが決まらない)
				//------------------------------------------------------
				Math::Vector3 _velValue(_vel.value);
				float _speedSq = _velValue.LengthSquared();
				if (!(_speedSq > 1e-8f)) continue;

				float         _speed   = std::sqrt(_speedSq);
				Math::Vector3 _curDir  = _velValue / _speed;

				//------------------------------------------------------
				// ターゲットへの向き
				//------------------------------------------------------
				Math::Vector3 _toTarget = _targetPos - Math::Vector3(_trs.pos);
				float _distSq = _toTarget.LengthSquared();
				if (!(_distSq > 1e-6f)) continue;	// ほぼ重なっている

				float _dist = std::sqrt(_distSq);

				// 追える距離を超えている間は曲げない(範囲に戻れば再び追う)
				if (_homing.searchRange > 0.0f && _dist > _homing.searchRange) continue;

				_toTarget /= _dist;

				//------------------------------------------------------
				// 今フレームで曲がれる角度まで進行方向を回す
				//------------------------------------------------------
				float _maxStep = _homing.turnSpeed * a_ctx.dt;	// ラジアン
				float _cos     = std::clamp(_curDir.Dot(_toTarget), -1.0f, 1.0f);
				float _angle   = std::acos(_cos);

				Math::Vector3 _newDir = _toTarget;
				if (_angle > _maxStep)
				{
					// 回転軸 = 現在の向き × 目標の向き
					Math::Vector3 _axis      = _curDir.Cross(_toTarget);
					float         _axisLenSq = _axis.LengthSquared();

					if (_axisLenSq > 1e-8f)
					{
						_axis /= std::sqrt(_axisLenSq);
					}
					else
					{
						// ちょうど真後ろ。軸が作れないので直交する適当な軸で回し始める
						Math::Vector3 _ref = (std::fabs(_curDir.y) > 0.99f)
							? Math::Vector3(1.0f, 0.0f, 0.0f)
							: Math::Vector3(0.0f, 1.0f, 0.0f);
						_axis = _curDir.Cross(_ref);
						_axis.Normalize();
					}

					Math::Quaternion _rot = Math::Quaternion::CreateFromAxisAngle(_axis, _maxStep);
					_newDir = Math::Vector3::Transform(_curDir, _rot);
					_newDir.Normalize();
				}

				// 弾速は変えず、向きだけ差し替える
				_vel.value = _newDir * _speed;
			}
		}
	);
}
