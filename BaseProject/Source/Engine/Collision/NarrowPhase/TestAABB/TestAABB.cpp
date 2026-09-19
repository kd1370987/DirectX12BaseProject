#include "TestAABB.h"

#include "../PrimitiveHelper/PrimitiveHelper.h"

namespace Engine::Collision::NarrowPhase
{
	bool TestAABB(const RayInfo& a_ray, const DirectX::BoundingBox& a_box, float& a_outDist)
	{
		// レイ情報とボックスの交差判定を行う
		return Math::DX::IntersectsRayAABB(a_ray, a_box, a_outDist);
	}
	bool TestAABB(const SphereInfo& a_info, const DirectX::BoundingBox& a_box, float& a_outDist)
	{
		a_outDist = 0.0f;
		return MakeSphere(a_info).Intersects(a_box);
	}
	bool TestAABB(const CapsuleInfo& a_info, const DirectX::BoundingBox& a_box, float& a_outDist)
	{
		// ブロードフェーズは保守的にカプセルのAABB同士で判定する
		a_outDist = 0.0f;
		return a_box.Intersects(MakeCapsuleAABB(a_info));
	}
}