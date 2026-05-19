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
}