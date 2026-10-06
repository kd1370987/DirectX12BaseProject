#pragma once

#include "Application/ECS/ISystem/APPISystem.h"

namespace App::System
{
	class EnemyMoveIntentSystem : public App::ECS::APPISystem
	{
	public:
		void Init(App::ECS::APPWorld& a_world) override;
	};
}
