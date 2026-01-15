#pragma once

class World;

struct TRSComponent;
struct ColliderComponent;
struct WorldMatrixComponent;
struct RayColliderComponent;
struct ModelComponent;

// コライダー情報ビュー
struct ColliderView
{
	ECS::Entity entity;
	TRSComponent* pTRS;
	ColliderComponent* pCollider;
	WorldMatrixComponent* pWorldMat;
	ModelComponent* pModelComp;
};

// レイコライダー情報ビュー
struct RayColliderView
{
	ECS::Entity entity;
	TRSComponent* pTRS;
	RayColliderComponent* pRayCollider;
	ColliderComponent* pCollider;
	bool isHit = false;
};

namespace Gather
{
	/// <summary>
	/// コライダー情報を配列で取得
	/// </summary>
	/// <param name="a_world">ワールドの参照</param>
	/// <param name="a_outColliderViewVec">出力先</param>
	void GatherColliderViews(
		World& a_world,
		std::vector<ColliderView>& a_outColliderViewVec
	);

	/// <summary>
	/// レイコライダー情報を配列で取得
	/// </summary>
	/// <param name="a_world">ワールドの参照</param>
	/// <param name="a_outRayColliderViewVec">出力先</param>
	void GatherRayColliderViews(
		World& a_world,
		std::vector<RayColliderView>& a_outRayColliderViewVec
	);

} // namespace Gather