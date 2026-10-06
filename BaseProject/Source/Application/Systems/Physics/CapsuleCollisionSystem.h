#pragma once

#include "Application/ECS/ISystem/APPISystem.h"

namespace App::System
{
	// カプセルコライダーのテスト用システム
	// カプセルをコリジョンワールドに問い合わせ、結果をデバッグワイヤーで表示する
	class CapsuleCollisionSystem : public App::ECS::APPISystem
	{
	public:

		void Init(App::ECS::APPWorld& a_world) override;
	};
}
