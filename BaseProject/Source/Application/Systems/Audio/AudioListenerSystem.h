#pragma once

#include "Application/ECS/ISystem/APPISystem.h"

namespace App::System
{
	// AudioListenerComponent を持つエンティティ(プレイヤー)の位置・向きを
	// 毎フレーム AudioManager へ送るシステム。
	class AudioListenerSystem : public App::ECS::APPISystem
	{
	public:

		void Init(App::ECS::APPWorld& a_world) override;
	};
}
