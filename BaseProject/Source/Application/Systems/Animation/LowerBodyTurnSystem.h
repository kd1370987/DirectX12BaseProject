#pragma once

#include "Application/ECS/ISystem/APPISystem.h"

namespace App::System
{
	// 腰から下だけを進行方向へ向ける(戦車のような脚と上半身の分離)
	class LowerBodyTurnSystem : public App::ECS::APPISystem
	{
	public:

		void Init(App::ECS::APPWorld& a_world) override;
	};
}
