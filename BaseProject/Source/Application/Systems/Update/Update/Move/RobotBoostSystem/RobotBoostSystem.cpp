#include "RobotBoostSystem.h"

#include "Engine/ECS/World/World.h"

#include "../../../../../Components/Force/VelocityComponent.h"
#include "../../../../../Components/Charactor/Robot/BoostComponent.h"

void RobotBoostSystem::Init(Engine::ECS::World& a_world)
{
	a_world.ActiveTask<BoostComponent, VelocityComponent>(
		Engine::ECS::ESystemType::Physics,
		"RobotBoostSystem",
		[]
		(
			Engine::ECS::ArchetypeChunk* a_pChunk,
			uint32_t a_count,
			const Engine::ECS::SystemContext& a_ctx,
			ActiveTag* a_tags,
			BoostComponent* a_boostArray,
			VelocityComponent* a_velArray
		)
		{
			for (size_t _i = 0; _i < a_count; ++_i)
			{
				BoostComponent& _boostComp = a_boostArray[_i];
				VelocityComponent& _velComp = a_velArray[_i];
				
				// 回復
				if (_boostComp.maxFuel >= _boostComp.currentFuel)
				{
					_boostComp.currentFuel += _boostComp.fuelRegeneration * a_ctx.dt;
				}

				// 使用量より燃料が下回っていたらブーストできない
				if (_boostComp.currentFuel <= _boostComp.boostFuel)
				{
					continue;
				}
				// ブースト押された瞬間
				else if (_boostComp.isBoostTriger)
				{
					_velComp.value.x *= _boostComp.boostPower;
					_velComp.value.y *= _boostComp.boostPower;
					_velComp.value.z *= _boostComp.boostPower;

					_boostComp.currentFuel -= _boostComp.boostFuel;
				}

				// ブーストが押されている最中
				if (_boostComp.isBoostIntent)
				{
					_velComp.value.x *= _boostComp.boostPower / 2;
					_velComp.value.y *= _boostComp.boostPower / 2;
					_velComp.value.z *= _boostComp.boostPower / 2;

					_boostComp.currentFuel -= _boostComp.boostFuelPerSec;
				}
			}
		}
	);
}
