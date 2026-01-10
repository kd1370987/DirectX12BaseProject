#pragma once

#include "../../Internal/SystemComon.h"

class World;

class ISystem
{
public:
	virtual void Update(World& a_world, float a_dt) = 0;
};