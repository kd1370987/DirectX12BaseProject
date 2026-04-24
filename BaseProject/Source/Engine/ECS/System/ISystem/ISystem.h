#pragma once

#include "../../Internal/SystemComon.h"

namespace Engine::ECS
{
	class World;

	class ISystem
	{
	public:
		virtual void Init(World& a_world) = 0;

		virtual void Update(World& a_world, float a_dt) = 0;
	};
}

