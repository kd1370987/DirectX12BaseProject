#include "BossCombatIntentSystem.h"

#include "Engine/ECS/World/World.h"

#include "../../../../Components/Character/Boss/BossComponent.h"
#include "../../../../Components/Character/TargetEntityComponent.h"
#include "../../../../Components/Character/LookAngleComponent.h"
#include "../../../../Components/Character/AimTargetPosComponent.h"
#include "../../../../Components/Character/Robot/BoostComponent.h"
#include "../../../../Components/Character/Robot/AttachmentSlotsComponent.h"
#include "../../../../Components/Character/Weapon/Gun/GunStateComponent.h"
#include "../../../../Components/Intent/MoveIntentComponent.h"
#include "../../../../Components/Intent/ActionIntentComponent.h"
#include "../../../../Components/Transform/LocalTransformComponent.h"
#include "../../../../Components/Transform/WorldMatrixComponent.h"
#include "../../../../Components/Force/MovementComponent.h"
#include "../../../../Components/Force/VelocityComponent.h"
#include "../../../../Components/Resource/StateMachineComponent.h"

#include <random>

//==========================================================================================
// BossCombatIntentSystem
//
// 人型ボスの行動を、プレイヤーの入力とまったく同じ形(視点角 + 移動入力 + ブースト入力 +
// 発射入力 + 狙点)へ落とす。プレイヤー側の InputMoveSystem / InputActionSystem に相当し、
// 「人の代わりにスティックとボタンを操作するもの」だと思えばよい。
//
// そのため動かす側は既存のシステムがそのまま使える。
//   LookAngle    → RotationSystem(機体の向き) / AdditivePoseSystem(上体の狙い)
//                  / RobotBoostSystem(ブーストの向き) / CharacterMovementSystem(移動の基準軸)
//   MoveIntent   → CharacterMovementSystem(視点基準 → 目標速度)
//   BoostComponent → RobotBoostSystem(推力)
//   ActionIntent → AttachmentDispatchSystem 経由で銃(子エンティティ)へ → GunShootSystem
//   AimTargetPos → 同上。銃はこの点へ向けて撃つ
// ボス用に増やしたのはこのシステムと BossMissileSalvoSystem の2つだけ。
//
// ・戦闘は距離ではなく命令で始まる
//     BossComponent::isCombatStarted が立つまで、その場で待機する(入力を全部 0 にする)。
//     命令を出すのは SceneSequence の BossOrder。単体で動きを見たいときは
//     BossComponent::startOnSpawn を立てておけば命令なしで始まる。
//
// ・機動(アーマードコア/オメガフェニックス風)
//     間合いは keepDistance ± keepMargin の幅で保つ。遠ければ詰め、近すぎれば下がり、
//     幅の内では前後に動かない。その間ずっと横へ流し続け(strafe)、向きは相手に固定する。
//     高さも keepHeight ± heightMargin で保つので、地面を歩かず空中に居座る。
//     さらに dashInterval ごとにブーストの単押し(クイックブースト)を入れて、
//     速度が一定にならないようにしている。
//
// ・行動パターン(EBossPattern)
//     patternDuration ごとに重み付き抽選で選び直す「今回はどう戦うか」。
//     懐へ飛び込む(Rush)・上を取る(HighGround)・下から来る(LowGround)・
//     回り込む(Orbit)・離れてミサイル(Retreat)。
//
//     パターンが差し替えるのは “どこに居たいか” と癖(横の強さ・切り返し/ダッシュの
//     間隔・射撃の傾向)だけで、そこへ行く手順は上の共通処理のまま。位置取りの目標を
//     すげ替えるだけで別の戦い方に見えるので、パターンごとに移動処理を書かなくて済む。
//     同じパターンが連続しないように抽選するので、待ち構えても読みが外れる。
//
// ・偏差撃ち
//     狙点は「相手の今の位置」ではなく「弾が届く頃の位置」。相手の実速度(MovementComponent)と
//     銃の弾速から先を読む。撃ち合いとして成立させるための最低限で、aimLeadScale = 0 に
//     すれば置き撃ちなしに戻せる。
//
// ・PreUpdate に置く理由
//     索敵結果(SearchPlayerSystem が書く TargetEntityComponent)を読むので同じ帯に居る。
//     ActionIntentComponent を書く側なので、それを読む AttachmentDispatchSystem より
//     自動的に前へ回る(RAW 依存)。狙点も同じタイミングで書けば同じフレームで子へ届く。
//==========================================================================================
namespace
{
	// 角度(度)を -180..180 へ畳む。視点角は加算し続けるので大きな値になり得る
	float WrapDeg180(float a_deg)
	{
		a_deg = std::fmod(a_deg + 180.0f, 360.0f);
		if (a_deg < 0.0f) a_deg += 360.0f;
		return a_deg - 180.0f;
	}

	// 現在角から目標角へ、1フレームぶんの上限つきで寄せる(度)
	float MoveTowardDeg(float a_current, float a_target, float a_maxDelta)
	{
		const float _diff = WrapDeg180(a_target - a_current);
		if (a_maxDelta <= 0.0f) return a_current + _diff;
		return a_current + std::clamp(_diff, -a_maxDelta, a_maxDelta);
	}

	// 間隔 ± 揺らぎ。同じ周期で動くと読まれてしまうので散らす
	float NextInterval(std::mt19937& a_rng, float a_base, float a_random)
	{
		if (a_random <= 0.0f) return std::max(a_base, 0.05f);

		std::uniform_real_distribution<float> _dist(-a_random, a_random);
		return std::max(a_base + _dist(a_rng), 0.05f);
	}

	//======================================================================================
	// 行動パターンの中身
	//--------------------------------------------------------------------------------------
	// パターンが差し替えるのは「どこに居たいか」と癖だけ。移動そのものの手順は
	// パターンによらず共通なので、ここでは目標と倍率を返すだけにしてある。
	//======================================================================================
	struct PatternProfile
	{
		float distance;				// 保ちたい間合い(m)
		float marginScale;			// 間合いの許容幅の倍率(小さいほどきっちり詰める)
		float height;				// 相手からの高さ(m。負で下)
		float strafeScale;			// 横移動の強さ倍率
		float strafeIntervalScale;	// 切り返し間隔の倍率(大きいほど同じ向きへ流し続ける)
		float dashIntervalScale;	// クイックブースト間隔の倍率(小さいほど頻繁)
		float gunRangeScale;		// 銃を撃ち始める距離の倍率
		float missileIntervalScale;	// 一斉射の間隔の倍率(小さいほど多く撒く)
	};

	PatternProfile MakeProfile(const BossComponent& a_boss, EBossPattern a_pattern)
	{
		switch (a_pattern)
		{
		case EBossPattern::Rush:
			// 懐へ飛び込む。横へ逃げずに真っ直ぐ詰めたいので切り返しは長め、
			// 代わりにクイックブーストを頻繁に入れて直線的に見せない
			return { a_boss.rushDistance, 0.5f, a_boss.rushHeight, 0.6f, 1.5f, 0.45f, 1.0f, 1.0f };

		case EBossPattern::HighGround:
			// 上を取って撃ち下ろす。距離は少し詰めて、ミサイルを多めに撒く
			return { a_boss.keepDistance * 0.8f, 1.0f, a_boss.highGroundHeight,
					 1.0f, 1.0f, 0.8f, 1.0f, 0.5f };

		case EBossPattern::LowGround:
			// 低く潜り込んで撃ち上げる。横の動きを強めて的を絞らせない
			return { a_boss.keepDistance * 0.7f, 1.0f, a_boss.lowGroundHeight,
					 1.3f, 0.8f, 0.7f, 1.0f, 1.0f };

		case EBossPattern::Orbit:
			// 間合いを保ったまま同じ向きへ回り込む。切り返しをほとんど挟まない
			return { a_boss.keepDistance, 1.0f, a_boss.keepHeight,
					 a_boss.orbitStrafeScale, 3.0f, 0.6f, 1.0f, 1.0f };

		case EBossPattern::Retreat:
			// 大きく離れてミサイル主体。銃は届かない距離なので撃たせない
			return { a_boss.retreatDistance, 1.5f, a_boss.keepHeight + 8.0f,
					 0.8f, 1.2f, 1.2f, 0.5f, 0.5f };

		case EBossPattern::Standoff:
		default:
			return { a_boss.keepDistance, 1.0f, a_boss.keepHeight, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f };
		}
	}

	//======================================================================================
	// 行動パターンの抽選
	//--------------------------------------------------------------------------------------
	// 重みつきのくじ引き。まず「今と違うパターン」だけで引き、そこで引けなければ
	// (重みが1つしか立っていない等)全体から引き直す。
	// 同じパターンが連続すると、詰めてくると分かった状態がそのまま続いてしまうため。
	//======================================================================================
	EBossPattern PickPattern(std::mt19937& a_rng, const BossComponent& a_boss, EBossPattern a_current)
	{
		constexpr int _kCount = static_cast<int>(EBossPattern::Max);

		const float _weights[_kCount] =
		{
			a_boss.weightStandoff,
			a_boss.weightRush,
			a_boss.weightHighGround,
			a_boss.weightLowGround,
			a_boss.weightOrbit,
			a_boss.weightRetreat,
		};

		for (int _pass = 0; _pass < 2; ++_pass)
		{
			// _pass == 0 : 今のパターンを除いて引く / _pass == 1 : 全部から引く
			const bool _isExcludeCurrent = (_pass == 0);

			float _total = 0.0f;
			for (int _i = 0; _i < _kCount; ++_i)
			{
				if (_weights[_i] <= 0.0f) continue;
				if (_isExcludeCurrent && _i == static_cast<int>(a_current)) continue;

				_total += _weights[_i];
			}
			if (_total <= 0.0f) continue;

			std::uniform_real_distribution<float> _dist(0.0f, _total);
			float _roll = _dist(a_rng);

			for (int _i = 0; _i < _kCount; ++_i)
			{
				if (_weights[_i] <= 0.0f) continue;
				if (_isExcludeCurrent && _i == static_cast<int>(a_current)) continue;

				_roll -= _weights[_i];
				if (_roll <= 0.0f) return static_cast<EBossPattern>(_i);
			}
		}

		// 重みが全部 0。動きが止まらないよう既定へ倒す
		return EBossPattern::Standoff;
	}
}

void BossCombatIntentSystem::Init(Engine::ECS::World& a_world)
{
	a_world.ActiveTask<
		const TargetEntityComponent,
		const LocalTransformComponent,
		const AttachmentSlotsComponent,
		BossComponent,
		LookAngleComponent,
		MoveIntentComponent,
		ActionIntentComponent,
		BoostComponent,
		AimTargetPosComponent>(
		Engine::ECS::ESystemType::PreUpdate,
		"BossCombatIntentSystem",
		[](
			Engine::ECS::ArchetypeChunk*      a_pChunk,
			uint32_t                          a_count,
			const Engine::ECS::SystemContext& a_ctx,
			ActiveTag*                        a_tags,
			const TargetEntityComponent*      a_targetArray,
			const LocalTransformComponent*    a_trsArray,
			const AttachmentSlotsComponent*   a_slotsArray,
			BossComponent*                    a_bossArray,
			LookAngleComponent*               a_lookArray,
			MoveIntentComponent*              a_moveIntentArray,
			ActionIntentComponent*            a_actionIntentArray,
			BoostComponent*                   a_boostArray,
			AimTargetPosComponent*            a_aimArray
		)
		{
			// 揺らぎ用の乱数。ECS はシングルスレッドで回るのでプロセス内に1つで足りる
			static std::mt19937 s_rng{ std::random_device{}() };
			static std::uniform_int_distribution<int> s_coin(0, 1);

			for (size_t _i = 0; _i < a_count; ++_i)
			{
				const TargetEntityComponent&    _target = a_targetArray[_i];
				const LocalTransformComponent&  _trs    = a_trsArray[_i];
				const AttachmentSlotsComponent& _slots  = a_slotsArray[_i];
				BossComponent&                  _boss   = a_bossArray[_i];
				LookAngleComponent&             _look   = a_lookArray[_i];
				MoveIntentComponent&            _intent = a_moveIntentArray[_i];
				ActionIntentComponent&          _action = a_actionIntentArray[_i];
				BoostComponent&                 _boost  = a_boostArray[_i];
				AimTargetPosComponent&          _aim    = a_aimArray[_i];

				// 命令を待たない設定なら、そのまま戦闘状態にしておく
				if (_boss.startOnSpawn) _boss.isCombatStarted = true;

				//==========================================================
				// 相手の位置と実速度を引く
				//==========================================================
				const Engine::ECS::Entity _targetEntity = _target.targetEntity;

				bool          _hasTarget = false;
				Math::Vector3 _targetPos = {};
				Math::Vector3 _targetVel = {};

				if (_targetEntity != Engine::ECS::Limits::INVALID_ENTITY &&
					a_ctx.pWorld->HasComponent<WorldMatrixComponent>(_targetEntity))
				{
					if (const auto* _pTargetWorld =
						a_ctx.pWorld->RefData<WorldMatrixComponent>(_targetEntity))
					{
						_targetPos = Math::Matrix(_pTargetWorld->worldMat).Translation();
						_hasTarget = true;
					}
				}

				if (_hasTarget)
				{
					// 偏差撃ち用。加減速を通した実速度(MovementComponent)を優先し、
					// 持っていなければ目標速度で代用する
					if (a_ctx.pWorld->HasComponent<MovementComponent>(_targetEntity))
					{
						if (const auto* _pMove = a_ctx.pWorld->RefData<MovementComponent>(_targetEntity))
						{
							_targetVel = _pMove->velocity;
						}
					}
					else if (a_ctx.pWorld->HasComponent<VelocityComponent>(_targetEntity))
					{
						if (const auto* _pVel = a_ctx.pWorld->RefData<VelocityComponent>(_targetEntity))
						{
							_targetVel = _pVel->value;
						}
					}
				}

				//==========================================================
				// 戦闘前 / 相手なし : その場で待つ
				//----------------------------------------------------------
				// 入力を全部 0 にするだけでよい。移動も発射もこの入力を見ている
				// システムが担当しているので、ここで止めれば何も起きない。
				// 重力は生きているので、浮いていれば降りて着地する。
				//==========================================================
				if (!_boss.isCombatStarted || !_hasTarget)
				{
					_boss.maneuver         = EBossManeuver::Wait;
					_boss.isMissileRequest = false;
					_boss.distance         = _hasTarget ? _target.distance : 0.0f;

					// 戦闘に入った最初のフレームでパターンを引き直させる
					_boss.patternTimer = 0.0f;

					_intent.value = {};

					_action.isGunShoot    = false;
					_action.isAiming      = false;
					_action.isMissileHold = false;

					_boost.isBoostIntent = false;
					_boost.isBoostTriger = false;
					continue;
				}

				const Math::Vector3 _selfPos(_trs.pos);

				//==========================================================
				// 行動パターンの抽選
				//----------------------------------------------------------
				// 一定時間ごとに「今回はどう戦うか」を引き直す。
				// 切り替えた瞬間は横の向きも選び直して、前のパターンの流れを引きずらせない。
				//==========================================================
				_boss.patternTimer -= a_ctx.dt;
				if (_boss.patternTimer <= 0.0f)
				{
					_boss.pattern      = PickPattern(s_rng, _boss, _boss.pattern);
					_boss.patternTimer =
						NextInterval(s_rng, _boss.patternDuration, _boss.patternDurationRand);

					_boss.strafeSign  = (s_coin(s_rng) == 0) ? -1.0f : 1.0f;
					_boss.strafeTimer = 0.0f;
				}

				const PatternProfile _profile = MakeProfile(_boss, _boss.pattern);

				//==========================================================
				// 狙点(偏差撃ち)
				//----------------------------------------------------------
				// 弾が届くまでの時間だけ相手の未来位置を読む。
				// 弾速はメイン武器の GunStateComponent から引く(弾ごとに変わるため)。
				//==========================================================
				Math::Vector3 _aimPos = _targetPos;
				_aimPos.y += _boss.aimOffsetY;

				const float _distance = (_aimPos - _selfPos).Length();
				_boss.distance = _distance;

				float _bulletSpeed = 0.0f;
				if (_slots.mainGun.id != Engine::ECS::Limits::INVALID_ENTITY &&
					a_ctx.pWorld->HasComponent<GunStateComponent>(_slots.mainGun.id))
				{
					if (const auto* _pGun = a_ctx.pWorld->RefData<GunStateComponent>(_slots.mainGun.id))
					{
						_bulletSpeed = _pGun->speed;
					}
				}

				if (_bulletSpeed > 0.0f && _boss.aimLeadScale > 0.0f)
				{
					const float _leadTime = (_distance / _bulletSpeed) * _boss.aimLeadScale;
					_aimPos += _targetVel * _leadTime;
				}

				// 狙いの向き。潰れたら今向いている方向のままにする
				Math::Vector3 _aimDir = _aimPos - _selfPos;
				const float   _aimLenSq = _aimDir.LengthSquared();
				if (_aimLenSq > 1e-6f) _aimDir /= std::sqrt(_aimLenSq);
				else                   _aimDir = { 0.0f, 0.0f, 1.0f };

				// AimTargetSystem(カメラのレイ)の代わりに自分で書く。
				// これで AttachmentDispatchSystem が銃へ配り、GunShootSystem が狙点へ撃つ
				_aim.pos       = _aimPos;
				_aim.dir       = _aimDir;
				_aim.hitEntity = Engine::ECS::Limits::INVALID_ENTITY;
				_aim.isHit     = false;
				_aim.isValid   = true;

				//==========================================================
				// 視点角を狙点へ寄せる
				//----------------------------------------------------------
				// 左手系 +Z 前方なので Yaw = atan2(x, z)。Pitch は上向きが正
				// (AdditivePoseSystem が符号を反転して使う側の規約に合わせている)。
				//==========================================================
				const float _desiredYaw   = DirectX::XMConvertToDegrees(std::atan2(_aimDir.x, _aimDir.z));
				const float _pitchLimit   = std::min(_boss.maxPitchDeg, _look.maxPitch);
				const float _desiredPitch = std::clamp(
					DirectX::XMConvertToDegrees(std::asin(std::clamp(_aimDir.y, -1.0f, 1.0f))),
					-_pitchLimit, _pitchLimit);

				_look.Yaw   = MoveTowardDeg(_look.Yaw, _desiredYaw, _boss.turnSpeedDeg * a_ctx.dt);
				_look.Pitch = MoveTowardDeg(_look.Pitch, _desiredPitch, _boss.pitchSpeedDeg * a_ctx.dt);
				_look.Pitch = std::clamp(_look.Pitch, -_look.maxPitch, _look.maxPitch);

				//==========================================================
				// 前後 : 間合いを幅で保つ
				//----------------------------------------------------------
				// 高さは別に見るので、前後の判断は水平距離で行う。
				// 3D距離で見ると、真上に居るだけで「近すぎる」と判断してしまう。
				//==========================================================
				Math::Vector3 _flat = _targetPos - _selfPos;
				_flat.y = 0.0f;
				const float _horizontalDist = _flat.Length();

				const float _margin = _boss.keepMargin * _profile.marginScale;
				const float _far    = _profile.distance + _margin;
				const float _near   = std::max(_profile.distance - _margin, 0.0f);

				float _forward = 0.0f;
				if (_horizontalDist > _far)
				{
					_forward       = _boss.approachThrottle;
					_boss.maneuver = EBossManeuver::Approach;
				}
				else if (_horizontalDist < _near)
				{
					_forward       = -_boss.backThrottle;
					_boss.maneuver = EBossManeuver::Back;
				}
				else
				{
					_boss.maneuver = EBossManeuver::Keep;
				}

				//==========================================================
				// 横 : 一定時間ごとに向きを切り替えて流し続ける
				//----------------------------------------------------------
				// 切り返しの間隔はパターン任せ。回り込み(Orbit)は長く、
				// 詰め(Rush)は真っ直ぐ行きたいので実質切り返さない。
				//==========================================================
				_boss.strafeTimer -= a_ctx.dt;
				if (_boss.strafeTimer <= 0.0f)
				{
					// たまに同じ向きへ続けて、切り返しの周期を読ませない
					_boss.strafeSign = (s_coin(s_rng) == 0) ? -_boss.strafeSign : _boss.strafeSign;
					if (_boss.strafeSign == 0.0f) _boss.strafeSign = 1.0f;

					_boss.strafeTimer = NextInterval(
						s_rng,
						_boss.strafeInterval * _profile.strafeIntervalScale,
						_boss.strafeIntervalRand);
				}
				const float _side = _boss.strafeSign * _boss.strafeThrottle * _profile.strafeScale;

				//==========================================================
				// 上下 : パターンが決めた高さに居座る
				//----------------------------------------------------------
				// ずれの大きさに比例させる。段階的に ±1 を入れると、境界で
				// 上昇と降下を往復して細かく震えてしまう。
				//==========================================================
				const float _heightError = (_targetPos.y + _profile.height) - _selfPos.y;

				float _up = 0.0f;
				if (std::fabs(_heightError) > _boss.heightMargin)
				{
					const float _range = std::max(_boss.heightMargin, 0.01f);
					_up = std::clamp(_heightError / (_range * 2.0f), -1.0f, 1.0f) * _boss.verticalThrottle;
				}

				//----------------------------------------------------------
				// 地面に着いている間は下へ押し込まない
				//----------------------------------------------------------
				// ブーストの向きは正規化されるので、下向き成分を入れたぶんだけ水平の
				// 推力が削られる。相手が地上に居るときの低空パターン(LowGround)は
				// 地面より下を目標にするので、そのままだと地面に押し付けたまま
				// 横へ動けなくなってしまう。
				//
				// 接地は StateMachineComponent(RayCollisionSystem が書く)で見るが、
				// クエリには足さない。PlayerIntentSystem がこれを書き、こちらが書く
				// MoveIntent / Boost を読む側でもあるので、依存が一周してしまう。
				//----------------------------------------------------------
				if (_up < 0.0f)
				{
					const Engine::ECS::Entity _self = a_pChunk->entityData[_i];
					if (a_ctx.pWorld->HasComponent<StateMachineComponent>(_self))
					{
						if (const auto* _pStateMachine =
							a_ctx.pWorld->RefData<StateMachineComponent>(_self))
						{
							if (_pStateMachine->isGround) _up = 0.0f;
						}
					}
				}

				// 視点基準の移動入力(プレイヤーのスティック入力と同じ意味)
				_intent.value.x = _side;
				_intent.value.y = _up;
				_intent.value.z = _forward;

				//==========================================================
				// ブースト
				//----------------------------------------------------------
				// 余力があるうちは吹かしっぱなしで飛び回り、間隔ごとに単押しを
				// 混ぜてクイックブーストをかける(初動だけ tapBoostScale が乗る)。
				// 残量が予備を切ったら休んで回復させる。
				//==========================================================
				const bool _canBoost =
					_boost.currentFuel > (_boost.boostFuel + _boss.boostFuelReserve);

				_boost.isBoostIntent = _canBoost;
				_boost.isBoostTriger = false;

				_boss.dashTimer -= a_ctx.dt;
				if (_boss.dashTimer <= 0.0f)
				{
					if (_canBoost) _boost.isBoostTriger = true;

					// 頻度もパターン任せ。詰めや回り込みでは短くして踏み込みを増やす
					_boss.dashTimer = NextInterval(
						s_rng,
						_boss.dashInterval * _profile.dashIntervalScale,
						_boss.dashIntervalRand);
				}

				//==========================================================
				// 銃 : 撃つ/休むを交互に。正面に入っていない間は撃たない
				//==========================================================
				_boss.gunTimer -= a_ctx.dt;
				if (_boss.gunTimer <= 0.0f)
				{
					_boss.isGunActive = !_boss.isGunActive;
					_boss.gunTimer    = std::max(
						_boss.isGunActive ? _boss.gunBurstTime : _boss.gunRestTime, 0.05f);
				}

				// 旋回が追いついていない間に撃つと明後日へ飛ぶので、正面に入るまで待つ
				const bool _isInCone =
					std::fabs(WrapDeg180(_desiredYaw - _look.Yaw)) <= _boss.gunConeDeg;

				// 撃ち始める距離もパターン任せ。離脱中(Retreat)は届かないので撃たない
				const float _gunRange = _boss.gunRange * _profile.gunRangeScale;

				_action.isGunShoot    = _boss.isGunActive && _isInCone && (_distance <= _gunRange);
				_action.isAiming      = (_distance <= _gunRange);
				// ミサイルは溜め撃ちではなく間隔で撃つので、この入力は使わない
				_action.isMissileHold = false;

				//==========================================================
				// ミサイル : 間隔ごとに一斉射を要求する
				//----------------------------------------------------------
				// 実際に撃つのは BossMissileSalvoSystem(PostUpdate)。発射位置に
				// ポッドの今フレームのワールド行列が要るため、あちらに任せている。
				// 射程外の間はタイマーを 0 で止めておき、入った瞬間に撃たせる。
				//==========================================================
				if (_boss.missileTimer > 0.0f)
				{
					_boss.missileTimer = std::max(_boss.missileTimer - a_ctx.dt, 0.0f);
				}

				if (_boss.missileTimer <= 0.0f &&
					!_boss.isMissileRequest &&
					_distance <= _boss.missileRange)
				{
					_boss.isMissileRequest = true;

					// 上を取ったときや離脱中はミサイル主体にしたいので、間隔もパターン任せ
					_boss.missileTimer = NextInterval(
						s_rng,
						_boss.missileInterval * _profile.missileIntervalScale,
						_boss.missileIntervalRand);
				}
			}
		}
	);
}
