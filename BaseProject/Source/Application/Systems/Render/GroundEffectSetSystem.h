#pragma once

#include "Application/ECS/ISystem/APPISystem.h"

namespace App::System
{
	class GroundEffectSetSystem : public App::ECS::APPISystem
	{
	public:


		void Init(App::ECS::APPWorld& a_world) override;
	};
}