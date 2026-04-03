#include "RotationSystem.h"

#include "Engine/ECS/World/World.h"

#include "Application/Components/Charactor/Player/PlayerLookAngleComponent.h"

#include "Application/Components/Transform/TRSComponent.h"

void RotationSystem::Run(Engine::ECS::World& a_world, float a_dt)
{
	a_world.ForEach<PlayerLookAngleComponent, TRSComponent>(
		[&a_world, a_dt]
		(
			Engine::ECS::ArchetypeChunk* a_pChunk,
			uint32_t a_count,
			PlayerLookAngleComponent* a_lookArray,
			TRSComponent* a_trsArray
			)
		{
			for (size_t _i = 0; _i < a_count; ++_i)
			{
				PlayerLookAngleComponent& _lookAng = a_lookArray[_i];
				TRSComponent& _trs = a_trsArray[_i];

				DirectX::XMVECTOR _quat = DirectX::XMQuaternionRotationAxis(
					DirectX::XMVectorSet(0,1,0,0),
					DirectX::XMConvertToRadians(_lookAng.Yaw)
				);

				DirectX::XMStoreFloat4(&_trs.quat,_quat);
			}
		}
	);
}
