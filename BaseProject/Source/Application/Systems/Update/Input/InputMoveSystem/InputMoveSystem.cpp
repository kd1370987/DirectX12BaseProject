#include "InputMoveSystem.h"

#include "Engine/ECS/World/World.h"

#include "../../../../Components/Force/VelocityComponent.h"
#include "../../../../Components/Force/InertiaComponent.h"

void InputMoveSystem::Run(World& a_world, float a_dt)
{
	DirectX::XMFLOAT3 inputDir = { 0.0f,0.0f,0.0f };
	if (GetAsyncKeyState(VK_UP) & 0x8000)
	{
		inputDir.y += 1.0f;
	}
	if (GetAsyncKeyState(VK_DOWN) & 0x8000)
	{

		inputDir.y -= 1.0f;
	}
	if (GetAsyncKeyState(VK_LEFT) & 0x8000)
	{
		inputDir.x -= 1.0f;

	}
	if (GetAsyncKeyState(VK_RIGHT) & 0x8000)
	{
		inputDir.x += 1.0f;
	}

	a_world.ForEachEx<VelocityComponent>
		(
			[&a_world, a_dt,inputDir]
			(
				ArchetypeChunk* a_pChunk,
				uint32_t a_count,
				VelocityComponent* a_velocityArray
				)
			{
				for (size_t _i = 0; _i < a_count; ++_i)
				{
					VelocityComponent& _velComp = a_velocityArray[_i];
					_velComp.value.x += inputDir.x;
					_velComp.value.y += inputDir.y;
				}
			},
			Exclude<InertiaComponent>()
		);
}
