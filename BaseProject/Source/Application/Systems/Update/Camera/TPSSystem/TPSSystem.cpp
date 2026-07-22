#include "TPSSystem.h"

#include "Engine/ECS/World/World.h"

#include "Application/Components/Camera/FollowTargetComponent.h"
#include "Application/Components/Camera/TPSOffsetComponent.h"
#include "../../../../Components/Camera/TPSCameraStateComponent.h"
#include "Application/Components/Transform/LocalTransformComponent.h"


#include "Application/Components/Camera/TPSLookAngleComponent.h"

#include "Application/Components/Charactor/Player/PlayerLookAngleComponent.h"


void TPSSystem::Init(Engine::ECS::World& a_world)
{
	a_world.ActiveTask<FollowTargetComponent, TPSOffsetComponent, TPSLookAngleComponent, LocalTransformComponent, TPSCameraStateComponent>(
		Engine::ECS::ESystemType::Camera,
		"TPSSystem",
		[&a_world](
			Engine::ECS::ArchetypeChunk* a_pChunk,
			uint32_t a_count,
			float a_dt,
			ActiveTag* a_tags,
			FollowTargetComponent* a_targetArray,
			TPSOffsetComponent* a_offsetArray,
			TPSLookAngleComponent* a_lookAngArray,
			LocalTransformComponent* a_trsArray,
			TPSCameraStateComponent* a_tpsStatArray
		)
		{
			for (size_t _i = 0; _i < a_count; ++_i)
			{
				// カメラのコンポーネントを取得
				FollowTargetComponent&		_followComp = a_targetArray[_i];
				TPSOffsetComponent&			_offsetComp = a_offsetArray[_i];
				LocalTransformComponent&	_trsComp	= a_trsArray[_i];
				TPSLookAngleComponent&		_lookComp	= a_lookAngArray[_i];
				TPSCameraStateComponent&	_statComp	= a_tpsStatArray[_i];

				//============================================================
				// ターゲット取得
				//============================================================
				Engine::ECS::Entity _target = _followComp.target;
				if (!a_world.HasComponent<LocalTransformComponent>(_target)) continue;
				if (!a_world.HasComponent<PlayerLookAngleComponent>(_target)) continue;
				const LocalTransformComponent* _targetTRS = a_world.RefData<LocalTransformComponent>(_target);
				const PlayerLookAngleComponent* _targetLook = a_world.RefData<PlayerLookAngleComponent>(_target);
				if (!_targetLook || !_targetTRS) continue;

				//============================================================
				// ターゲット回転
				//------------------------------------------------------------
				// Yaw/Pitchは度(degree)で保持されているのでラジアンへ変換する。
				// Vector3オーバーロードは(pitch,yaw,roll)順で軸が入れ替わるため、
				// スカラー版 CreateFromYawPitchRoll(yaw,pitch,roll) を明示的に使う。
				//============================================================
				DXSM::Quaternion _targetRot = DXSM::Quaternion::CreateFromYawPitchRoll(
					DirectX::XMConvertToRadians(_targetLook->Yaw),
					DirectX::XMConvertToRadians(-_targetLook->Pitch),
					0.0f
				);
				_targetRot.Normalize();

				//============================================================
				// ピボット / 距離 / 注視点
				//============================================================
				DXSM::Vector3 _pivot = _targetTRS->pos + DXSM::Vector3::Up * _offsetComp.y;
				float _distance = _offsetComp.z;
				DXSM::Vector3 _targetLookAt = _targetTRS->pos + DXSM::Vector3(0.f, 1.5f, 0.f);

				//============================================================
				// オービット回転の補間(クォータニオンSlerp)
				//------------------------------------------------------------
				// Vector3::Lerp+正規化での「向き」補間は、現在向きと目標向きが
				// ほぼ反対を向いた瞬間に補間結果がゼロ近傍を通り、正規化が破綻して
				// 高速に180度反転する。Slerpは最短経路で回るためこの破綻が無い。
				//============================================================
				float _t = std::min(10.0f * a_dt, 1.0f);		// 補間係数

				DXSM::Quaternion _curOrbit = _statComp.currentOrbit;
				if (_curOrbit.LengthSquared() < 1e-6f) _curOrbit = _targetRot;	// 未初期化保険

				DXSM::Quaternion _orbit = DXSM::Quaternion::Slerp(_curOrbit, _targetRot, _t);
				_orbit.Normalize();
				_statComp.currentOrbit = _orbit;

				//============================================================
				// カメラ位置：ピボットの後方へ距離d(常に球面上=距離固定)
				//============================================================
				DXSM::Vector3 _dir = DXSM::Vector3::Transform(DXSM::Vector3::Backward, _orbit); // (0,0,1)を回転
				DXSM::Vector3 _currentPos = _pivot + _dir * _distance;

				//============================================================
				// カメラ回転(このエンジンは左手系。CreateLookAtは右手系で
				// 向きが180度反転するため XMMatrixLookAtLH を使う)
				//============================================================
				DXSM::Matrix _view = DirectX::XMMatrixLookAtLH(
					_currentPos,
					_targetLookAt,
					DXSM::Vector3::Up
				);
				DXSM::Quaternion _camRot = DXSM::Quaternion::CreateFromRotationMatrix(_view.Invert());

				//============================================================
				// 保存
				//============================================================
				_trsComp.pos = _currentPos;
				_trsComp.quat = _camRot;
				_statComp.currentLookAt = _targetLookAt;
				_trsComp.isDirty = true;
			}
		}
	);
}
