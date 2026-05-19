#pragma once
namespace Engine::Collision::NarrowPhase
{
	// プリミティブ vs AABB
	// 純粋な数学的交差判定のみを行う関数群

	// レイ vs AABB : 当たった距離が取れる
	bool TestAABB(const RayInfo& a_ray,const DirectX::BoundingBox& a_box,float& a_outDist);

}