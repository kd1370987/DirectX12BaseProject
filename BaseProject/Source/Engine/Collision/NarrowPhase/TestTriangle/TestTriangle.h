#pragma once
namespace Engine::Collision::NarrowPhase
{
	// プリミティブ vs 三角形
	// 純粋な数学的交差判定のみを行う関数群

	// レイ vs 三角形 : 当たった距離が取れる
	bool TestTriangle(
		const RayInfo& a_ray,
		const DirectX::XMVECTOR& a_v0,
		const DirectX::XMVECTOR& a_v1,
		const DirectX::XMVECTOR& a_v2,
		float& a_outDist
	);

	// 以下のオーバーラップ系プリミティブは交差の有無のみを返す（a_outDist は 0 固定）

	// 球 vs 三角形
	bool TestTriangle(
		const SphereInfo& a_info,
		const DirectX::XMVECTOR& a_v0,
		const DirectX::XMVECTOR& a_v1,
		const DirectX::XMVECTOR& a_v2,
		float& a_outDist
	);

	// カプセル vs 三角形
	bool TestTriangle(
		const CapsuleInfo& a_info,
		const DirectX::XMVECTOR& a_v0,
		const DirectX::XMVECTOR& a_v1,
		const DirectX::XMVECTOR& a_v2,
		float& a_outDist
	);

	// OBB vs 三角形
	bool TestTriangle(
		const OBBInfo& a_info,
		const DirectX::XMVECTOR& a_v0,
		const DirectX::XMVECTOR& a_v1,
		const DirectX::XMVECTOR& a_v2,
		float& a_outDist
	);

	// フラスタム vs 三角形
	bool TestTriangle(
		const FrustumInfo& a_info,
		const DirectX::XMVECTOR& a_v0,
		const DirectX::XMVECTOR& a_v1,
		const DirectX::XMVECTOR& a_v2,
		float& a_outDist
	);
}