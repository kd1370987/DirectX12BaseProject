#include "TestAABB.h"

#include "../PrimitiveHelper/PrimitiveHelper.h"

namespace Engine::Collision::NarrowPhase
{
	bool TestAABB(const RayInfo& a_ray, const DirectX::BoundingBox& a_box, float& a_outDist)
	{
		// レイ情報とボックスの交差判定を行う
		DirectX::XMVECTOR _rayOrigin = DirectX::XMLoadFloat3(&a_ray.origin);
		DirectX::XMVECTOR _direction = DirectX::XMLoadFloat3(&a_ray.direction);
		return a_box.Intersects(_rayOrigin, _direction, a_outDist);
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
	bool TestAABB(const OBBInfo& a_info, const DirectX::BoundingBox& a_box, float& a_outDist)
	{
		a_outDist = 0.0f;
		return MakeOBB(a_info).Intersects(a_box);
	}
	bool TestAABB(const FrustumInfo& a_info, const DirectX::BoundingBox& a_box, float& a_outDist)
	{
		a_outDist = 0.0f;
		return MakeFrustum(a_info).Intersects(a_box);
	}
}