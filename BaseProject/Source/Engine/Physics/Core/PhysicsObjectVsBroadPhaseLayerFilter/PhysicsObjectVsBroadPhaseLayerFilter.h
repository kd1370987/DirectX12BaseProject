#pragma once

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Collision/ObjectLayer.h>
#include <Jolt/Physics/PhysicsSystem.h>

namespace Engine::Physics
{
	class PhysicsObjectVsBroadPhaseLayerFilter : public JPH::ObjectVsBroadPhaseLayerFilter
	{
	public:

		bool ShouldCollide(JPH::ObjectLayer a_layer1, JPH::BroadPhaseLayer a_layer2) const override;
	};

}