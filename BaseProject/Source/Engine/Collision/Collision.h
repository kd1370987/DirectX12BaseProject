#pragma once

struct Result;
struct ColliderView;
struct RayColliderView;

namespace Collision
{
	bool Raycast(
		const RayColliderView& a_ray,
		const std::vector<ColliderView>& a_colliderViewVec,
		Result& a_outResult
	);
}