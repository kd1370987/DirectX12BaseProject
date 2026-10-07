#pragma once

#include "Application/ECS/ISystem/APPISystem.h"

namespace App::System
{
	// HUD が読む値を集めて PlayerHUDResource へ書き込むシステム。
	class HUDGatherSystem : public App::ECS::APPISystem
	{
	public:

		void Init(App::ECS::APPWorld& a_world) override;
	};
}
