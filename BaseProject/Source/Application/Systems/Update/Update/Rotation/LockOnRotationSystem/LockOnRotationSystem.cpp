#include "LockOnRotationSystem.h"

#include "Application/ECS/World/World.h"
#include "Engine/Resource/Data/ActionStateMachineAsset/ActionStateMachineAsset.h"

#include "Application/Components/Tag/PlayerControllTag.h"
#include "Application/Components/Character/LookAngleComponent.h"
#include "Application/Components/Character/AimTargetPosComponent.h"
#include "Application/Components/Character/LockOnTargetComponent.h"
#include "Application/Components/Transform/LocalTransformComponent.h"
#include "../../../../../Components/Force/VelocityComponent.h"
#include "../../../../../Components/Resource/ActionStateComponent.h"

//==============================================================================
// LockOnRotationSystem
//
// プレイヤー専用の旋回システム。
// 現在の ActionState(ActionNode)を見て「体をどちらへ向けるか」を切り替える。
//
//   ・EFaceMode::AimDirection … 狙点(AimTargetPosComponent)の方向を体全体で向く。
//                               銃を撃つ/敵をターゲットしているステートで使う。
//   ・EFaceMode::MoveDirection … 進行方向を向く(従来の RotationSystem と同じ挙動)。
//   ・EFaceMode::Keep          … 旋回しない。
//   ・canRotate == false       … モードに関わらず旋回しない(マスタースイッチ)。
//
// ただしロック対象(LockOnTargetComponent.lockedEntity ＝ HUD で赤枠になっている敵)が
// 居る間は、モードより優先してその相手を向く。ロックしているのに進行方向を向いて
// 背中を見せる、という見え方を避けるため。
// 「旋回しない」指定(Keep / canRotate == false)はロックより強い。
//
// 旋回は Y 軸まわり(Yaw)のみ。上下に傾かないよう方向は水平化する。
// 上体だけの追従は AdditivePoseSystem が別に持っているので、
// ここで機体を向ければ「体で向く + 上体で微調整」の二段構えになる。
//
// 汎用の RotationSystem は PlayerControllTag を除外しているので姿勢は競合しない。
// 逆に言うと、プレイヤーの姿勢はこのシステムが全部を持つ。そのため
// ステートマシンを積んでいないプレイヤー用に「従来通り」のタスクも登録しておく。
//==============================================================================
namespace
{
	// 旋回速度の既定値(従来の RotationSystem と同じ)
	constexpr float kDefaultTurnSpeed = 12.0f;

	//--------------------------------------------------------------------------
	// 水平方向のベクトルから Yaw を作る。
	// このエンジンは左手系でローカル +Z が前方なので Yaw = atan2(x, z)。
	// 真上/真下・ゼロ長・NaN の場合は false を返して呼び出し側で旋回を諦める。
	//--------------------------------------------------------------------------
	bool CalcYawFromDir(const Math::Vector3& a_dir, float& a_outYaw)
	{
		Math::Vector3 _flat(a_dir.x, 0.0f, a_dir.z);

		// 長さのチェックは NaN も弾ける形で書くこと
		float _lenSq = _flat.LengthSquared();
		if (!(_lenSq > 1e-6f)) return false;

		_flat /= std::sqrt(_lenSq);
		a_outYaw = std::atan2(_flat.x, _flat.z);
		return true;
	}

	//--------------------------------------------------------------------------
	// 目標 Yaw へ Slerp で滑らかに旋回させる
	//--------------------------------------------------------------------------
	void ApplyYawSlerp(LocalTransformComponent& a_trs, float a_targetYaw, float a_turnSpeed, float a_dt)
	{
		Math::Quaternion _targetQuat = Math::Quaternion::CreateFromYawPitchRoll(a_targetYaw, 0.0f, 0.0f);
		_targetQuat.Normalize();

		Math::Quaternion _currentQuat(a_trs.quat);
		if (_currentQuat.LengthSquared() < 1e-6f) _currentQuat = Math::Quaternion::Identity();

		float _t = std::clamp(a_turnSpeed * a_dt, 0.0f, 1.0f);
		_currentQuat = Math::Quaternion::Slerp(_currentQuat, _targetQuat, _t);
		_currentQuat.Normalize();

		a_trs.quat = _currentQuat;
		a_trs.isDirty = true;	// 停止中でも行列を再構築させる
	}
}

void LockOnRotationSystem::Init(App::ECS::World& a_world)
{
	//==========================================================================
	// ステートマシンを持つプレイヤー(本命)
	//==========================================================================
	a_world.ActiveTask<
		const PlayerControllTag,
		const ActionStateComponent,
		const LookAngleComponent,
		const VelocityComponent,
		LocalTransformComponent>(
		Engine::ECS::ESystemType::Update,
		"LockOnRotationSystem",
		[]
		(
			Engine::ECS::ArchetypeChunk* a_pChunk,
			uint32_t a_count,
			const Engine::ECS::SystemContext& a_ctx,
			ActiveTag* a_tags,
			const PlayerControllTag* a_playerTagArray,
			const ActionStateComponent* a_stateArray,
			const LookAngleComponent* a_lookArray,
			const VelocityComponent* a_velocityArray,
			LocalTransformComponent* a_trsArray
		)
		{
			using namespace Engine::Resource;

			for (size_t _i = 0; _i < a_count; ++_i)
			{
				const ActionStateComponent&	_state		= a_stateArray[_i];
				const LookAngleComponent&	_lookAng	= a_lookArray[_i];
				const VelocityComponent&	_velComp	= a_velocityArray[_i];
				LocalTransformComponent&	_trs		= a_trsArray[_i];

				//==============================================================
				// 現在のステートから向きの決め方を取り出す
				//--------------------------------------------------------------
				// 設計図やノードが取れなくても止まらないように、
				// 取れなければ従来通り「進行方向を向く」で扱う。
				//==============================================================
				EFaceMode	_faceMode	= EFaceMode::MoveDirection;
				float		_turnSpeed	= kDefaultTurnSpeed;
				float		_yawOffset	= 0.0f;

				const auto* _pSM = a_ctx.pServices->pResourceManager->Get(_state.actionHandle);
				if (_pSM)
				{
					if (const ActionNode* _pNode = _pSM->GetStateNode(_state.currentStateHash))
					{
						// このステート中は向きを変えられない
						if (!_pNode->canRotate) continue;

						_faceMode	= _pNode->faceMode;
						_turnSpeed	= _pNode->turnSpeed;
						_yawOffset	= _pNode->faceYawOffsetDeg;
					}
				}

				if (_faceMode == EFaceMode::Keep) continue;

				//==============================================================
				// 目標 Yaw を求める
				//==============================================================
				float _targetYaw = 0.0f;

				Engine::ECS::Entity _self = a_pChunk->entityData[_i];

				//--------------------------------------------------------------
				// ロック中の相手が居ればそちらを最優先で向く。
				// ロック結果は PostUpdate(LockOnTargetSystem)が書くので 1 フレーム前の
				// 値だが、どのみち Slerp で追従させるので体感差は出ない。
				//--------------------------------------------------------------
				bool _hasLockYaw = false;
				if (a_ctx.pWorld->HasComponent<LockOnTargetComponent>(_self))
				{
					if (const auto* _pLockOn = a_ctx.pWorld->RefData<LockOnTargetComponent>(_self))
					{
						if (_pLockOn->IsLocked())
						{
							_hasLockYaw = CalcYawFromDir(
								Math::Vector3(_pLockOn->lockedPos) - Math::Vector3(_trs.pos), _targetYaw);
						}
					}
				}

				if (_hasLockYaw)
				{
					// ロック優先。下の faceMode 別の処理は飛ばす
				}
				else if (_faceMode == EFaceMode::AimDirection)
				{
					//----------------------------------------------------------
					// 狙っている方向。
					//
					// 狙点(AimTargetPosComponent)は AimTargetSystem が Camera フェーズで
					// 書くので 1 フレーム前の値になるが、どのみち Slerp で追従させるので
					// 体感差は出ない。狙点を持たない構成でも動くよう、
					// 取れなければ視線角(=カメラの向き)で代用する。
					//----------------------------------------------------------
					bool _hasYaw = false;

					if (a_ctx.pWorld->HasComponent<AimTargetPosComponent>(_self))
					{
						if (const auto* _pAim = a_ctx.pWorld->RefData<AimTargetPosComponent>(_self))
						{
							if (_pAim->isValid)
							{
								_hasYaw = CalcYawFromDir(
									Math::Vector3(_pAim->pos) - Math::Vector3(_trs.pos), _targetYaw);

								// 狙点が真上/真下・自機と同一座標。狙いの向きで代用する
								if (!_hasYaw)
								{
									_hasYaw = CalcYawFromDir(Math::Vector3(_pAim->dir), _targetYaw);
								}
							}
						}
					}

					if (!_hasYaw)
					{
						_targetYaw = DirectX::XMConvertToRadians(_lookAng.Yaw);
					}
				}
				else
				{
					//----------------------------------------------------------
					// 進行方向(従来の挙動)。
					// 停止中は目標が作れないので今の向きのままにする。
					//----------------------------------------------------------
					if (!CalcYawFromDir(Math::Vector3(_velComp.value), _targetYaw)) continue;
				}

				_targetYaw += DirectX::XMConvertToRadians(_yawOffset);

				ApplyYawSlerp(_trs, _targetYaw, _turnSpeed, a_ctx.dt);
			}
		}
	);

	//==========================================================================
	// ステートマシンを持たないプレイヤー(従来通り進行方向を向くだけ)
	//--------------------------------------------------------------------------
	// RotationSystem が PlayerControllTag を除外している以上、
	// ここで拾わないと一切旋回しなくなってしまう。
	//==========================================================================
	a_world.ActiveTask<
		const PlayerControllTag,
		const VelocityComponent,
		LocalTransformComponent>(
		Engine::ECS::ESystemType::Update,
		"LockOnRotationSystem_NoActionState",
		[]
		(
			Engine::ECS::ArchetypeChunk* a_pChunk,
			uint32_t a_count,
			const Engine::ECS::SystemContext& a_ctx,
			ActiveTag* a_tags,
			const PlayerControllTag* a_playerTagArray,
			const VelocityComponent* a_velocityArray,
			LocalTransformComponent* a_trsArray
		)
		{
			for (size_t _i = 0; _i < a_count; ++_i)
			{
				const VelocityComponent&	_velComp	= a_velocityArray[_i];
				LocalTransformComponent&	_trs		= a_trsArray[_i];

				float _targetYaw = 0.0f;
				if (!CalcYawFromDir(Math::Vector3(_velComp.value), _targetYaw)) continue;

				ApplyYawSlerp(_trs, _targetYaw, kDefaultTurnSpeed, a_ctx.dt);
			}
		},
		Engine::ECS::Exclude<ActionStateComponent>()
	);
}
