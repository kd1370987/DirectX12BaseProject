#include "TPSSystem.h"

#include "Engine/ECS/World/World.h"

#include "Application/Components/Camera/FollowTargetComponent.h"
#include "Application/Components/Camera/TPSOffsetComponent.h"

#include "Application/Components/Transform/TransformComponent.h"


#include "Application/Components/Camera/TPSLookAngleComponent.h"

#include "Application/Components/Charactor/Player/PlayerLookAngleComponent.h"


void TPSSystem::Run(Engine::ECS::World& a_world, float a_dt)
{
	a_world.ForEach<FollowTargetComponent,TPSOffsetComponent,TPSLookAngleComponent, TransformComponent>(
		[&a_world, a_dt]
		(
			Engine::ECS::ArchetypeChunk* a_pChunk,
			uint32_t a_count,
			FollowTargetComponent* a_targetArray,
			TPSOffsetComponent* a_offsetArray,
			TPSLookAngleComponent* a_lookAngArray,
			TransformComponent* a_trsArray
			)
		{
			for (size_t _i = 0; _i < a_count; ++_i)
			{
				Engine::ECS::Entity _target = a_targetArray[_i].target;
				const TransformComponent* _targetTRS = a_world.RefData<TransformComponent>(_target);
				const PlayerLookAngleComponent* _targetLook = a_world.RefData<PlayerLookAngleComponent>(_target);
				TransformComponent& _trsComp = a_trsArray[_i];

				// 視点
				TPSLookAngleComponent& _lookAng = a_lookAngArray[_i];
				_lookAng.Pitch = std::clamp(
					_lookAng.Pitch,
					-_lookAng.ClampPitch, 
					_lookAng.ClampPitch
				);

				DirectX::XMVECTOR _quat = DirectX::XMQuaternionRotationRollPitchYaw(
					DirectX::XMConvertToRadians(_lookAng.Pitch),
					DirectX::XMConvertToRadians(_targetLook->Yaw),
					0.0f
				);

				_quat = DirectX::XMQuaternionNormalize(_quat);

				DirectX::XMStoreFloat4(&_trsComp.quat,_quat);

				// プレイヤーの後ろ方向を算出
				DirectX::XMVECTOR _forward = DirectX::XMVector3Rotate(
					DirectX::XMVectorSet(0,0,1,0),
					_quat
				);

				DirectX::XMFLOAT3 _forw;
				DirectX::XMStoreFloat3(&_forw,_forward);


				float _distance = a_offsetArray[_i].z;

				DirectX::XMVECTOR _camPos =
				DirectX::XMVectorAdd(DirectX::XMVectorAdd(DirectX::XMLoadFloat3(&_targetTRS->pos), DirectX::XMVectorScale(_forward, _distance)), DirectX::XMVectorScale(DirectX::XMVectorSet(0, 1, 0, 0), a_offsetArray[_i].y));
				

				// 移動
				DirectX::XMStoreFloat3(&_trsComp.pos, _camPos);


			}
		}
	);
}
