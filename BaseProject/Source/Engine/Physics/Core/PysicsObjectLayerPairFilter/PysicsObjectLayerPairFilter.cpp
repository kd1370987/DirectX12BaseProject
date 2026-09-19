#include "PysicsObjectLayerPairFilter.h"

#include "../PhysicsLayer.h"

namespace Engine::Physics
{
	bool PysicsObjectLayerPairFilter::ShouldCollide(JPH::ObjectLayer a_layer1, JPH::ObjectLayer a_layer2) const
	{
		// 動かないもの同士は衝突させない
		if (!Layer::IsMoving(a_layer1) && !Layer::IsMoving(a_layer2))
		{
			return false;
		}

		// シミュレーションでぶつけ合うのは、お互いが相手を当たりに行く対象にしているときだけ。
		// (クエリは LayerMaskQueryFilter でクエリ側のマスクだけを見る)
		return (Layer::GetGroup(a_layer1) & Layer::GetMask(a_layer2)) != 0 &&
			(Layer::GetGroup(a_layer2) & Layer::GetMask(a_layer1)) != 0;
	}
}
