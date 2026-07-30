#include "RobotBoostSystem.h"

#include "Engine/ECS/World/World.h"

#include "../../../../../Components/Force/VelocityComponent.h"
#include "../../../../../Components/Character/Robot/BoostComponent.h"

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

				// ブーストが押されている最中の推力
				const float _holdPower = _boostComp.boostPower / 2;

				// ブースト押された瞬間
				// 押した瞬間と押しっぱなしは同じフレームで両方成立するので、初動を優先して二重に掛からないようにする
				if (_boostComp.isBoostTriger)
				{
					// 初動は tapBoostScale の分だけ大きめの推力を出す
					const float _tapPower = _holdPower * _boostComp.tapBoostScale;

					_velComp.value.x *= _tapPower;
					_velComp.value.y *= _tapPower;
					_velComp.value.z *= _tapPower;

					_boostComp.currentFuel -= _boostComp.boostFuel;
				}
				// ブーストが押されている最中
				else if (_boostComp.isBoostIntent)
				{
					_velComp.value.x *= _holdPower;
					_velComp.value.y *= _holdPower;
					_velComp.value.z *= _holdPower;

					_boostComp.currentFuel -= _boostComp.boostFuelPerSec * a_ctx.dt;
				}
			}
		}
	);
}
