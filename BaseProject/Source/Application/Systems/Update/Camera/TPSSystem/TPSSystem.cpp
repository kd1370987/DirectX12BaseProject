#include "TPSSystem.h"

#include "Engine/ECS/World/World.h"

#include "Application/Components/Camera/FollowTargetComponent.h"
#include "Application/Components/Camera/TPSOffsetComponent.h"
#include "../../../../Components/Camera/TPSCameraStateComponent.h"
#include "Application/Components/Transform/TransformComponent.h""


#include "Application/Components/Camera/TPSLookAngleComponent.h"

#include "Application/Components/Charactor/Player/PlayerLookAngleComponent.h"


void TPSSystem::Init(Engine::ECS::World& a_world)
{
	a_world.ActiveTask<FollowTargetComponent, TPSOffsetComponent, TPSLookAngleComponent, TransformComponent,TPSCameraStateComponent>(
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
			TransformComponent* a_trsArray,
			TPSCameraStateComponent* a_tpsStatArray
		) 
		{
			for (size_t _i = 0; _i < a_count; ++_i)
			{
				// カメラのコンポーネントを取得
				FollowTargetComponent&	_followComp = a_targetArray[_i];
				TPSOffsetComponent&		_offsetComp = a_offsetArray[_i];
				TransformComponent&		_trsComp	= a_trsArray[_i];
				TPSLookAngleComponent&	_lookComp	= a_lookAngArray[_i];
				TPSCameraStateComponent& _statComp	= a_tpsStatArray[_i];

				// ターゲットのコンポーネントを取得
				Engine::ECS::Entity _target = _followComp.target;
				const TransformComponent* _targetTRS = a_world.RefData<TransformComponent>(_target);
				const PlayerLookAngleComponent* _targetLook = a_world.RefData<PlayerLookAngleComponent>(_target);
				if (!_targetLook || !_targetTRS) continue;
				
				DirectX::XMVECTOR _quat = DirectX::XMQuaternionRotationRollPitchYaw(
					DirectX::XMConvertToRadians(-_targetLook->Pitch),
					DirectX::XMConvertToRadians(_targetLook->Yaw),
					0.0f
				);

				_quat = DirectX::XMQuaternionNormalize(_quat);

				DirectX::XMStoreFloat4(&_trsComp.quat, _quat);

				// プレイヤーの後ろ方向を算出
				DirectX::XMVECTOR _forward = DirectX::XMVector3Rotate(
					DirectX::XMVectorSet(0, 0, 1, 0),
					_quat
				);

				DirectX::XMFLOAT3 _forw;
				DirectX::XMStoreFloat3(&_forw, _forward);


				float _distance = _offsetComp.z;

				// カメラの目標座標を計算する
				DirectX::XMVECTOR _targetCamPos =
					DirectX::XMVectorAdd(DirectX::XMVectorAdd(DirectX::XMLoadFloat3(&_targetTRS->pos), 
						DirectX::XMVectorScale(_forward, _distance)), 
						DirectX::XMVectorScale(DirectX::XMVectorSet(0, 1, 0, 0), _offsetComp.y)
					);

				// カメラの注視点を計算する
				DirectX::XMVECTOR _targetLookAt = DirectX::XMVectorAdd(
					DirectX::XMLoadFloat3(&_targetTRS->pos),
					DirectX::XMVectorSet(0.0f, 1.5f, 0.0f, 0.0f) // 少し上のオフセット
				);

				// リープを使って、徐々に目標へ近づける
				float _lerpSpeed = 10.0f * a_dt;

				DirectX::XMVECTOR _currentPos = DirectX::XMLoadFloat3(&_trsComp.pos);
				_currentPos = DirectX::XMVectorLerp(_currentPos,_targetCamPos,_lerpSpeed);

				DirectX::XMVECTOR _currentLookAt = DirectX::XMLoadFloat3(&_statComp.currentLookAt);
				_currentLookAt = DirectX::XMVectorLerp(_currentLookAt, _targetLookAt, _lerpSpeed);

				DirectX::XMMATRIX _viewMat = DirectX::XMMatrixLookAtLH(
					_currentPos,
					_currentLookAt,
					DirectX::XMVectorSet(0, 1, 0, 0)
				);

				// View行列からカメラのワールドクォータニオンを抽出する
				DirectX::XMVECTOR _det;
				DirectX::XMMATRIX _invViewMat = DirectX::XMMatrixInverse(&_det, _viewMat);

				DirectX::XMVECTOR _camQuat = DirectX::XMQuaternionRotationMatrix(_invViewMat);
				DirectX::XMStoreFloat4(&_trsComp.quat, _camQuat);

				// 状態の保存と移動の適用
				DirectX::XMStoreFloat3(&_statComp.currentLookAt, _currentLookAt);
				DirectX::XMStoreFloat3(&_trsComp.pos, _currentPos);
			}
		}
	);
}