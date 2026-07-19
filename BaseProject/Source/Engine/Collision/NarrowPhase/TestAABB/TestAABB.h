#pragma once
namespace Engine::Collision::NarrowPhase
{
	// プリミティブ vs AABB
	// 純粋な数学的交差判定のみを行う関数群

	// レイ vs AABB : 当たった距離が取れる
	bool TestAABB(const RayInfo& a_ray,const DirectX::BoundingBox& a_box,float& a_outDist);

	// 以下のオーバーラップ系プリミティブは交差の有無のみを返す（a_outDist は 0 固定）

	// 球 vs AABB
	bool TestAABB(const SphereInfo& a_info,const DirectX::BoundingBox& a_box,float& a_outDist);

	// カプセル vs AABB
	bool TestAABB(const CapsuleInfo& a_info,const DirectX::BoundingBox& a_box,float& a_outDist);

	// OBB vs AABB
	bool TestAABB(const OBBInfo& a_info,const DirectX::BoundingBox& a_box,float& a_outDist);

	// フラスタム vs AABB
	bool TestAABB(const FrustumInfo& a_info,const DirectX::BoundingBox& a_box,float& a_outDist);

}