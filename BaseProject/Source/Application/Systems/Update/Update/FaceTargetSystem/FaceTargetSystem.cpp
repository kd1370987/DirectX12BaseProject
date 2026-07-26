#include "FaceTargetSystem.h"

#include "Engine/ECS/World/World.h"

#include "../../../../Components/Character/TargetEntityComponent.h"
#include "../../../../Components/Transform/LocalTransformComponent.h"
#include "../../../../Components/Transform/WorldMatrixComponent.h"

//==============================================================================
// FaceTargetSystem
//
// プレイヤーを視認している敵(TargetEntityComponent.isFind == true)を、
// プレイヤーの方向へ滑らかに旋回させる。
//
// ・旋回は Y 軸まわり(Yaw)のみ。上下に傾かないよう方向は水平化する。
// ・このエンジンは左手系でローカル +Z が前方。RotationSystem と同じく
//   Yaw = atan2(dir.x, dir.z) で目標角を作り、Slerp で追従する。
// ・RotationSystem は PlayerLookAngleComponent 保持者(プレイヤー)専用なので、
//   敵の quat をここで書いても競合しない。
// ・Update 帯に置く。書いた quat は PostUpdate の行列計算で反映される。
//==============================================================================
void FaceTargetSystem::Init(Engine::ECS::World& a_world)
{
	a_world.ActiveTask<const TargetEntityComponent, LocalTransformComponent>(
		Engine::ECS::ESystemType::Update,
		"FaceTargetSystem",
		[](
			Engine::ECS::ArchetypeChunk*      a_pChunk,
			uint32_t                          a_count,
			const Engine::ECS::SystemContext& a_ctx,
			ActiveTag*                        a_tags,
			const TargetEntityComponent*      a_targetArray,
			LocalTransformComponent*          a_trsArray
		)
		{
			// 旋回速度(1秒あたりの補間強度。RotationSystem に合わせた値)
			constexpr float _kTurnSpeed = 10.0f;

			for (size_t _i = 0; _i < a_count; ++_i)
			{
				const TargetEntityComponent& _target = a_targetArray[_i];
				LocalTransformComponent&     _trs    = a_trsArray[_i];

				// 視認していないときは旋回しない
				if (!_target.isFind) continue;
				if (_target.targetEntity == Engine::ECS::Limits::INVALID_ENTITY) continue;
				if (!a_ctx.pWorld->HasComponent<WorldMatrixComponent>(_target.targetEntity)) continue;

				const auto* _pPlayerWorld =
					a_ctx.pWorld->RefData<WorldMatrixComponent>(_target.targetEntity);
				if (!_pPlayerWorld) continue;

				DXSM::Vector3 _playerPos = DXSM::Matrix(_pPlayerWorld->worldMat).Translation();

				// 対象への水平方向(Y を無視)
				DXSM::Vector3 _dir = _playerPos - DXSM::Vector3(_trs.pos);
				_dir.y = 0.0f;

				float _lenSq = _dir.LengthSquared();
				if (!(_lenSq > 1e-6f)) continue;	// 真上/真下・同一座標は旋回不能
				_dir /= std::sqrt(_lenSq);

				// 左手系 +Z 前方: Yaw = atan2(x, z)
				float _targetYaw = std::atan2(_dir.x, _dir.z);
				DirectX::XMVECTOR _targetQuat =
					DirectX::XMQuaternionRotationRollPitchYaw(0.0f, _targetYaw, 0.0f);

				// 現在の姿勢から Slerp で滑らかに追従
				DirectX::XMVECTOR _currentQuat = DirectX::XMLoadFloat4(&_trs.quat);
				float _t = std::min(_kTurnSpeed * a_ctx.dt, 1.0f);
				_currentQuat = DirectX::XMQuaternionSlerp(_currentQuat, _targetQuat, _t);

				DirectX::XMStoreFloat4(&_trs.quat, _currentQuat);
				_trs.isDirty = true;	// 停止中でも行列を再構築させる
			}
		}
	);
}
