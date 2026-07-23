#include "RotationSystem.h"

#include "Engine/ECS/World/World.h"

#include "Application/Components/Charactor/Player/PlayerLookAngleComponent.h"
#include "Application/Components/Transform/LocalTransformComponent.h"
#include "../../../../../Components/Force/VelocityComponent.h"

void RotationSystem::Init(Engine::ECS::World& a_world)
{
	a_world.ActiveTask<const PlayerLookAngleComponent, LocalTransformComponent,const VelocityComponent>(
		Engine::ECS::ESystemType::Update,
		"RotationSystem",
		[]
		(
			Engine::ECS::ArchetypeChunk* a_pChunk,
			uint32_t a_count,
			const Engine::ECS::SystemContext& a_ctx,
			ActiveTag* a_tags,
			const PlayerLookAngleComponent* a_lookArray,
			LocalTransformComponent* a_trsArray,
			const VelocityComponent* a_velocityArray
		)
		{
			for (size_t _i = 0; _i < a_count; ++_i)
			{
				const PlayerLookAngleComponent& _lookAng = a_lookArray[_i];
				LocalTransformComponent& _trs = a_trsArray[_i];
				const VelocityComponent& _velComp = a_velocityArray[_i];

				// Y軸の移動を無視した平面での移動ベクトル取得
				DirectX::XMVECTOR _velocity = DirectX::XMLoadFloat3(&_velComp.value);
				_velocity = DirectX::XMVectorSetY(_velocity, 0.0f);

				// 移動している場合のみ機体を旋回させる
				if (DirectX::XMVectorGetX(DirectX::XMVector3LengthSq(_velocity)) > 0.001f)
				{
					// 移動方向のベクトルを正規化
					DirectX::XMVECTOR _forward = DirectX::XMVector3Normalize(_velocity);

					// Z軸とX軸から、目標となるYaw角を計算
					float _targetYaw = atan2f(
						DirectX::XMVectorGetX(_forward),
						DirectX::XMVectorGetZ(_forward)
					);

					// 目標のクォータニオンを作成
					DirectX::XMVECTOR _targetQuat = DirectX::XMQuaternionRotationRollPitchYaw(0.0f, _targetYaw, 0.0f);

					// 球面線形補間を使って滑らかに旋回させる
					float _slerpSpeed = 12.0f * a_ctx.dt;
					DirectX::XMVECTOR _currentQuat = DirectX::XMLoadFloat4(&_trs.quat);

					_currentQuat = DirectX::XMQuaternionSlerp(_currentQuat, _targetQuat, _slerpSpeed);

					// 結果を保存
					DirectX::XMStoreFloat4(&_trs.quat, _currentQuat);
				}
				else
				{
					// ※ロックオン時や射撃時など、移動していなくてもカメラや敵の方向を
					// 強制的に向かせたい場合は、ここに別のSlerp処理を書くことになります。
				}
			}
		}
	);
}
