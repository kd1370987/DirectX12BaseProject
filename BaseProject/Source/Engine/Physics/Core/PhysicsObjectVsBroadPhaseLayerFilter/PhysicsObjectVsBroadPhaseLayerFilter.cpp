#include "PhysicsObjectVsBroadPhaseLayerFilter.h"

#include "../PhysicsLayer.h"

namespace Engine::Physics
{
	bool PhysicsObjectVsBroadPhaseLayerFilter::ShouldCollide(JPH::ObjectLayer a_layer1, JPH::BroadPhaseLayer a_layer2) const
	{
		// 動かないもの同士は調べない。
		// それ以外の細かい絞り込み(group と mask)は ObjectLayerPairFilter が受け持つ
		if (!Layer::IsMoving(a_layer1))
		{
			return a_layer2 == EBroadPhaseLayer::Dynamic;
		}
		return true;
	}
}
