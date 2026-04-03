#pragma once

#include "../../Internal/SystemComon.h"

namespace Engine::ECS
{
	class World;

	class ISystem
	{
	public:
		virtual void Update(World& a_world, float a_dt) = 0;
	};
}

