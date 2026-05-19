#include "TestAABB.h"
namespace Engine::Collision::NarrowPhase
{
	bool TestAABB(const RayInfo& a_ray, const DirectX::BoundingBox& a_box, float& a_outDist)
	{
		// レイ情報とボックスの交差判定を行う
		DirectX::XMVECTOR _rayOrigin = DirectX::XMLoadFloat3(&a_ray.origin);
		DirectX::XMVECTOR _direction = DirectX::XMLoadFloat3(&a_ray.direction);
		return a_box.Intersects(_rayOrigin, _direction, a_outDist);
	}
}