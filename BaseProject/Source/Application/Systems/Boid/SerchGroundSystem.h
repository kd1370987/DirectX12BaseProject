#pragma once

#include "Application/ECS/ISystem/APPISystem.h"

namespace App::System
{
	class SerchGroundSystem : public App::ECS::APPISystem
	{
	public:
		void Init(App::ECS::APPWorld& a_world) override;
	};
}
