#pragma once

struct Result;
struct ColliderView;
struct RayColliderView;

class Mesh;

namespace Engine
{
	namespace Collision
	{
		bool Raycast(
			const RayColliderView& a_ray,
			const std::vector<ColliderView>& a_colliderViewVec,
			Result& a_outResult
		);

		// レイ判定
		namespace Ray
		{
			// モデル
			bool VSModel(
				const Collider::RayInfo& a_rayInfo,
				const Engine::Resource::Model* a_pModel,
				const DirectX::XMFLOAT4X4& a_worldMat,
				Result& a_outResult
			);

			// メッシュ
			bool VSMesh(
				const Collider::RayInfo& a_rayInfo,
				const Engine::Resource::Mesh* a_pMesh,
				const DirectX::XMFLOAT4X4& a_worldMat,
				Result& a_outResult
			);

			// ポリゴン
			bool VSPolygon(
				const Collider::RayInfo& a_rayInfo,
				const DirectX::XMFLOAT4X4& a_worldMat,
				Result& a_outResult
			);
		}
	}
}