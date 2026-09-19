#include "PhysicsBroadPhaseLayer.h"

#include "../PhysicsLayer.h"

namespace Engine::Physics
{
	JPH::uint PhysicsBroadPhaseLayer::GetNumBroadPhaseLayers() const
	{
		return static_cast<JPH::uint>(EBroadPhaseLayer::NumLayers);
	}

	JPH::BroadPhaseLayer PhysicsBroadPhaseLayer::GetBroadPhaseLayer(JPH::ObjectLayer a_layer) const
	{
		switch (static_cast<EObjectLayer>(a_layer))
		{
		case EObjectLayer::NonMoving:
			return static_cast<JPH::BroadPhaseLayer>(EBroadPhaseLayer::Static);

		case EObjectLayer::Moving:
			return static_cast<JPH::BroadPhaseLayer>(EBroadPhaseLayer::Dynamic);

		default:
			JPH_ASSERT(false);
			return static_cast<JPH::BroadPhaseLayer>(EBroadPhaseLayer::Static);
		}
	}
}