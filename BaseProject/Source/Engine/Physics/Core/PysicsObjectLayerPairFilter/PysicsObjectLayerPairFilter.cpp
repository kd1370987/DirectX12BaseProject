#include "PysicsObjectLayerPairFilter.h"

#include "../PhysicsLayer.h"

namespace Engine::Physics
{
	bool PysicsObjectLayerPairFilter::ShouldCollide(JPH::ObjectLayer a_layer1, JPH::ObjectLayer a_layer2) const
	{
		const EObjectLayer layer1 = static_cast<EObjectLayer>(a_layer1);
		const EObjectLayer layer2 = static_cast<EObjectLayer>(a_layer2);

		// Static × Static は衝突させない
		if (layer1 == EObjectLayer::NonMoving && layer2 == EObjectLayer::NonMoving)
		{
			return false;
		}

		return true;
	}
}