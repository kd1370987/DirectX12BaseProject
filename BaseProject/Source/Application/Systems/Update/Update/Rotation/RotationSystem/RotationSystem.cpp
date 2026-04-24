#include "RotationSystem.h"

#include "Engine/ECS/World/World.h"

#include "Application/Components/Charactor/Player/PlayerLookAngleComponent.h"

#include "Application/Components/Transform/TransformComponent.h"

void RotationSystem::Init(Engine::ECS::World& a_world)
{
	a_world.RegisterTask<const PlayerLookAngleComponent, TransformComponent>(
		Engine::ECS::ESystemType::Update,
		[]
		(
			Engine::ECS::ArchetypeChunk* a_pChunk,
			uint32_t a_count,
			float a_dt,
			const PlayerLookAngleComponent* a_lookArray,
			TransformComponent* a_trsArray
		)
		{
			for (size_t _i = 0; _i < a_count; ++_i)
			{
				const PlayerLookAngleComponent& _lookAng = a_lookArray[_i];
				TransformComponent& _trs = a_trsArray[_i];

				DirectX::XMVECTOR _quat = DirectX::XMQuaternionRotationAxis(
					DirectX::XMVectorSet(0, 1, 0, 0),
					DirectX::XMConvertToRadians(_lookAng.Yaw)
				);

				DirectX::XMStoreFloat4(&_trs.quat, _quat);
			}
		}
	);
}

void RotationSystem::Run(Engine::ECS::World& a_world, float a_dt)
{
	a_world.ForEach<PlayerLookAngleComponent, TransformComponent>(
		[&a_world, a_dt]
		(
			Engine::ECS::ArchetypeChunk* a_pChunk,
			uint32_t a_count,
			PlayerLookAngleComponent* a_lookArray,
			TransformComponent* a_trsArray
			)
		{
			for (size_t _i = 0; _i < a_count; ++_i)
			{
				PlayerLookAngleComponent& _lookAng = a_lookArray[_i];
				TransformComponent& _trs = a_trsArray[_i];

				DirectX::XMVECTOR _quat = DirectX::XMQuaternionRotationAxis(
					DirectX::XMVectorSet(0,1,0,0),
					DirectX::XMConvertToRadians(_lookAng.Yaw)
				);

				DirectX::XMStoreFloat4(&_trs.quat,_quat);
			}
		}
	);
}
