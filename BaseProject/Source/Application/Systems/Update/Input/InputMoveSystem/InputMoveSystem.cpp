#include "InputMoveSystem.h"

#include "Engine/ECS/World/World.h"

#include "Application/Components/Force/VelocityComponent.h"
#include "Application/Components/Force/InertiaComponent.h"

#include "Application/Components/Tag/PlayerControllTag.h"

#include "Application/Components/Charactor/Player/PlayerLookAngleComponent.h"

void InputMoveSystem::Run(Engine::ECS::World& a_world, float a_dt)
{
	DirectX::XMFLOAT3 inputDir = { 0.0f,0.0f,0.0f };
	float inputLook = 0.0f;
	if (GetAsyncKeyState('W') & 0x8000)
	{
		inputDir.z += 1.0f;
	}
	if (GetAsyncKeyState('S') & 0x8000)
	{

		inputDir.z -= 1.0f;
	}
	if (GetAsyncKeyState('A') & 0x8000)
	{
		inputDir.x -= 1.0f;

	}
	if (GetAsyncKeyState('D') & 0x8000)
	{
		inputDir.x += 1.0f;
	}

	if (GetAsyncKeyState(VK_LEFT) & 0x8000)
	{
		inputLook -= 1.0f;
	}
	if (GetAsyncKeyState(VK_RIGHT) & 0x8000)
	{
		inputLook += 1.0f;
	}

	a_world.ForEachEx<PlayerControllTag,VelocityComponent,PlayerLookAngleComponent>
		(
			[&a_world, a_dt,inputDir,inputLook]
			(
				Engine::ECS::ArchetypeChunk* a_pChunk,
				uint32_t a_count,
				PlayerControllTag* a_tags,
				VelocityComponent* a_velocityArray,
				PlayerLookAngleComponent* a_playerLookArray
				)
			{
				for (size_t _i = 0; _i < a_count; ++_i)
				{
					PlayerLookAngleComponent& _lookComp = a_playerLookArray[_i];
					_lookComp.Yaw += inputLook;

					float _rad = DirectX::XMConvertToRadians(_lookComp.Yaw);
					float _sinY = sinf(_rad);
					float _cosY = cosf(_rad);

					VelocityComponent& _velComp = a_velocityArray[_i];
					_velComp.value.x += (inputDir.x * _cosY + inputDir.z * _sinY) * 5.0f;
					_velComp.value.y += inputDir.y;
					_velComp.value.z += (inputDir.z * _cosY - inputDir.x * _sinY) * 5.0f;
				}
			},
			Engine::ECS::Exclude<InertiaComponent>()
		);
}
