#include "PhysicsObjectVsBroadPhaseLayerFilter.h"

#include "../PhysicsLayer.h"
namespace Engine::Physics
{
	bool PhysicsObjectVsBroadPhaseLayerFilter::ShouldCollide(JPH::ObjectLayer a_layer1, JPH::BroadPhaseLayer a_layer2) const
	{
		switch (static_cast<EObjectLayer>(a_layer1))
		{
		case EObjectLayer::NonMoving:
			return a_layer2 == EBroadPhaseLayer::Dynamic;

		case EObjectLayer::Moving:
			return true;

		default:
			JPH_ASSERT(false);
			return false;
		}
	}
}
