#include "TPSSystem.h"

#include "Engine/ECS/World/World.h"

#include "Application/Components/Camera/FollowTargetComponent.h"
#include "Application/Components/Camera/TPSOffsetComponent.h"
#include "../../../../Components/Camera/TPSCameraStateComponent.h"
#include "../../../../Components/Camera/CameraForcusTargetComponent.h"
#include "Application/Components/Transform/LocalTransformComponent.h"


#include "Application/Components/Camera/TPSLookAngleComponent.h"

#include "Application/Components/Charactor/Player/PlayerLookAngleComponent.h"


void TPSSystem::Init(Engine::ECS::World& a_world)
{
	a_world.ActiveTask<FollowTargetComponent, TPSOffsetComponent, TPSLookAngleComponent, LocalTransformComponent, TPSCameraStateComponent>(
		Engine::ECS::ESystemType::Camera,
		"TPSSystem",
		[](
			Engine::ECS::ArchetypeChunk* a_pChunk,
			uint32_t a_count,
			const Engine::ECS::SystemContext& a_ctx,
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
				if (!a_ctx.pWorld->HasComponent<LocalTransformComponent>(_target)) continue;
				if (!a_ctx.pWorld->HasComponent<PlayerLookAngleComponent>(_target)) continue;
				if (!a_ctx.pWorld->HasComponent<CameraForcusTargetComponent>(_target)) continue;
				const LocalTransformComponent* _targetTRS = a_ctx.pWorld->RefData<LocalTransformComponent>(_target);
				const PlayerLookAngleComponent* _targetLook = a_ctx.pWorld->RefData<PlayerLookAngleComponent>(_target);
				const CameraForcusTargetComponent* _forcusTarget = a_ctx.pWorld->RefData<CameraForcusTargetComponent>(_target);
				if (!_targetLook || !_targetTRS || !_forcusTarget) continue;

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
				DXSM::Vector3 _targetLookAt = DXSM::Vector3(_targetTRS->pos) + DXSM::Vector3(_forcusTarget->offsetPos);

				//============================================================
				// オービット回転の補間(クォータニオンSlerp)
				//------------------------------------------------------------
				// Vector3::Lerp+正規化での「向き」補間は、現在向きと目標向きが
				// ほぼ反対を向いた瞬間に補間結果がゼロ近傍を通り、正規化が破綻して
				// 高速に180度反転する。Slerpは最短経路で回るためこの破綻が無い。
				//============================================================
				float _t = std::min(10.0f * a_ctx.dt, 1.0f);		// 補間係数

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
				//------------------------------------------------------------
				// XMMatrixLookAtLH は「視点==注視点」や「視線がUpと平行」の場合に
				// 内部の正規化/外積が破綻して行列全体が NaN になる。
				// NaN のクォータニオンが LocalTransform に入ると、そこから前方ベクトルを
				// 求める側(AimTargetSystem など)まで NaN が伝播して落ちるため、
				// 破綻する条件では回転を更新せず前フレームの値を保つ。
				//============================================================
				DXSM::Vector3 _lookVec = _targetLookAt - _currentPos;
				float _lookLenSq = _lookVec.LengthSquared();
				bool _isDegenerate = (_lookLenSq < 1e-6f);
				if (!_isDegenerate)
				{
					// 視線がUpとほぼ平行（真上/真下を向いている）かどうか
					DXSM::Vector3 _lookDir = _lookVec / std::sqrt(_lookLenSq);
					_isDegenerate = (std::fabs(_lookDir.Dot(DXSM::Vector3::Up)) > 0.9999f);
				}
				// 破綻時は前フレームの回転を維持する(位置だけは更新する)
				DXSM::Quaternion _camRot = DXSM::Quaternion(_trsComp.quat);
				if (!_isDegenerate)
				{
					DXSM::Matrix _view = DirectX::XMMatrixLookAtLH(
						_currentPos,
						_targetLookAt,
						DXSM::Vector3::Up
					);
					_camRot = DXSM::Quaternion::CreateFromRotationMatrix(_view.Invert());
					_camRot.Normalize();
				}

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
