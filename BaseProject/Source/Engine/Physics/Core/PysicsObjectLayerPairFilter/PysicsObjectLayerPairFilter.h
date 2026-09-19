#pragma once

#include <Jolt/Jolt.h>
#include <Jolt/Physics/PhysicsSystem.h>

namespace Engine::Physics
{
	class PysicsObjectLayerPairFilter : public JPH::ObjectLayerPairFilter
	{
	public:
		bool ShouldCollide(JPH::ObjectLayer a_layer1, JPH::ObjectLayer a_layer2) const override;
	};
}