#include "FaceTargetSystem.h"

#include "Engine/ECS/World/World.h"

#include "../../../../Components/Character/TargetEntityComponent.h"
#include "../../../../Components/Character/LookAngleComponent.h"
#include "../../../../Components/Transform/LocalTransformComponent.h"
#include "../../../../Components/Transform/WorldMatrixComponent.h"
#include "../../../../Components/Resource/ActionStateComponent.h"

#include "Engine/Resource/Data/ActionStateMachineAsset/ActionStateMachineAsset.h"

//==============================================================================
// FaceTargetSystem
//
// 戦闘モードの敵(TargetEntityComponent.isFind == true)を、
// プレイヤーの方向へ滑らかに旋回させる。
//
// ・旋回は Y 軸まわり(Yaw)のみ。上下に傾かないよう方向は水平化する。
// ・このエンジンは左手系でローカル +Z が前方。RotationSystem と同じく
//   Yaw = atan2(dir.x, dir.z) で目標角を作り、Slerp で追従する。
// ・姿勢を書く他システムとの住み分け:
//     RotationSystem       … LookAngleComponent 保持者(プレイヤー以外)
//     LockOnRotationSystem … PlayerControllTag 保持者
//   ザコ敵は LookAngleComponent を持たないので、ここで quat を書いても競合しない。
//   逆に LookAngleComponent を持つ側(ボスなど)は、視線角を自分で更新して姿勢の
//   書き込みは RotationSystem に任せる作りになっているので、ここでは除外する。
//   除外しないと同じ quat を2つのシステムが書き、実行順が登録順頼みになってしまう。
// ・行動ステートの canRotate == false のあいだは旋回しない(LockOnRotationSystem と同じ)。
//   死亡ステートのように「もう向きを変えない」状態を、アセット側の設定だけで
//   表せるようにするため。設計図やノードが取れないときは従来どおり旋回する。
// ・Update 帯に置く。書いた quat は PostUpdate の行列計算で反映される。
//==============================================================================
void FaceTargetSystem::Init(Engine::ECS::World& a_world)
{
	a_world.ActiveTask<const TargetEntityComponent, const ActionStateComponent, LocalTransformComponent>(
		Engine::ECS::ESystemType::Update,
		"FaceTargetSystem",
		[](
			Engine::ECS::ArchetypeChunk*      a_pChunk,
			uint32_t                          a_count,
			const Engine::ECS::SystemContext& a_ctx,
			ActiveTag*                        a_tags,
			const TargetEntityComponent*      a_targetArray,
			const ActionStateComponent*       a_stateArray,
			LocalTransformComponent*          a_trsArray
		)
		{
			// 旋回速度(1秒あたりの補間強度。RotationSystem に合わせた値)
			constexpr float _kTurnSpeed = 10.0f;

			for (size_t _i = 0; _i < a_count; ++_i)
			{
				const TargetEntityComponent& _target = a_targetArray[_i];
				const ActionStateComponent&  _state  = a_stateArray[_i];
				LocalTransformComponent&     _trs    = a_trsArray[_i];

				// このステート中は向きを変えられない(死亡ステートなど)
				if (const auto* _pSM = a_ctx.pServices->pResourceManager->Get(_state.actionHandle))
				{
					const auto* _pNode = _pSM->GetStateNode(_state.currentStateHash);
					if (_pNode && !_pNode->canRotate) continue;
				}

				// 視認していないときは旋回しない
				if (!_target.isFind) continue;
				if (_target.targetEntity == Engine::ECS::Limits::INVALID_ENTITY) continue;
				if (!a_ctx.pWorld->HasComponent<WorldMatrixComponent>(_target.targetEntity)) continue;

				const auto* _pPlayerWorld =
					a_ctx.pWorld->RefData<WorldMatrixComponent>(_target.targetEntity);
				if (!_pPlayerWorld) continue;

				Math::Vector3 _playerPos = Math::Matrix(_pPlayerWorld->worldMat).Translation();

				// 対象への水平方向(Y を無視)
				Math::Vector3 _dir = _playerPos - Math::Vector3(_trs.pos);
				_dir.y = 0.0f;

				float _lenSq = _dir.LengthSquared();
				if (!(_lenSq > 1e-6f)) continue;	// 真上/真下・同一座標は旋回不能
				_dir /= std::sqrt(_lenSq);

				// 左手系 +Z 前方: Yaw = atan2(x, z)
				float _targetYaw = std::atan2(_dir.x, _dir.z);
				const Math::Quaternion _targetQuat =
					Math::Quaternion::CreateFromYawPitchRoll(_targetYaw, 0.0f, 0.0f);

				// 現在の姿勢から Slerp で滑らかに追従
				const float _t = std::min(_kTurnSpeed * a_ctx.dt, 1.0f);
				_trs.quat = Math::Quaternion::Slerp(_trs.quat, _targetQuat, _t);
				_trs.isDirty = true;	// 停止中でも行列を再構築させる
			}
		},
		Engine::ECS::Exclude<LookAngleComponent>{}
	);
}
