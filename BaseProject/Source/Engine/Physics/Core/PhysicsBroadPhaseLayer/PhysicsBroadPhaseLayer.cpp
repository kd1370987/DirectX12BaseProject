#include "PhysicsBroadPhaseLayer.h"

#include "../PhysicsLayer.h"

namespace Engine::Physics
{
	JPH::uint PhysicsBroadPhaseLayer::GetNumBroadPhaseLayers() const
	{
		return EBroadPhaseLayer::NumLayers;
	}

	JPH::BroadPhaseLayer PhysicsBroadPhaseLayer::GetBroadPhaseLayer(JPH::ObjectLayer a_layer) const
	{
		// 動くかどうかの印だけで振り分ける。
		// 静的な地形はツリーを作り直さずに済み、毎フレーム動く弾やボイドとは別のツリーに乗る
		return Layer::IsMoving(a_layer) ? EBroadPhaseLayer::Dynamic : EBroadPhaseLayer::Static;
	}
}
