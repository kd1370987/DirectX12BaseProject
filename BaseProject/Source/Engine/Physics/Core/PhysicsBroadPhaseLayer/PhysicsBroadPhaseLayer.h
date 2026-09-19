#pragma once

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Collision/BroadPhase/BroadPhaseLayer.h>

namespace Engine::Physics
{
	class PhysicsBroadPhaseLayer : public JPH::BroadPhaseLayerInterface
	{
	public:
		JPH::uint GetNumBroadPhaseLayers() const override;

		JPH::BroadPhaseLayer GetBroadPhaseLayer(JPH::ObjectLayer a_layer) const override;

//#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
//		const char* GetBroadPhaseLayerName(JPH::BroadPhaseLayer a_layer) const override;
//#endif
	};
}