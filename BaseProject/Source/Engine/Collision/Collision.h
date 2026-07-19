#pragma once

struct Result;
struct ColliderView;
struct RayColliderView;

namespace Engine
{
	namespace Collision
	{

		// レイ判定
		namespace Ray
		{
			// モデル
			bool VSModel(
				const RayInfo& a_rayInfo,
				const Engine::Resource::Model* a_pModel,
				const DirectX::XMFLOAT4X4& a_worldMat,
				Result& a_outResult
			);

			// メッシュ
			bool VSMesh(
				const RayInfo& a_rayInfo,
				const Engine::Resource::Mesh* a_pMesh,
				const DirectX::XMFLOAT4X4& a_worldMat,
				Result& a_outResult
			);
		}

		// 球判定（オーバーラップ : 触れているかどうか）
		namespace Sphere
		{
			bool VSModel(const SphereInfo& a_info, const Engine::Resource::Model* a_pModel, const DirectX::XMFLOAT4X4& a_worldMat, Result& a_outResult);
			bool VSMesh(const SphereInfo& a_info, const Engine::Resource::Mesh* a_pMesh, const DirectX::XMFLOAT4X4& a_worldMat, Result& a_outResult);
		}

		// カプセル判定
		namespace Capsule
		{
			bool VSModel(const CapsuleInfo& a_info, const Engine::Resource::Model* a_pModel, const DirectX::XMFLOAT4X4& a_worldMat, Result& a_outResult);
			bool VSMesh(const CapsuleInfo& a_info, const Engine::Resource::Mesh* a_pMesh, const DirectX::XMFLOAT4X4& a_worldMat, Result& a_outResult);
		}

		// OBB判定（軸並行BoxもこのOBB経路を通す）
		namespace OBB
		{
			bool VSModel(const OBBInfo& a_info, const Engine::Resource::Model* a_pModel, const DirectX::XMFLOAT4X4& a_worldMat, Result& a_outResult);
			bool VSMesh(const OBBInfo& a_info, const Engine::Resource::Mesh* a_pMesh, const DirectX::XMFLOAT4X4& a_worldMat, Result& a_outResult);
		}

		// フラスタム判定
		namespace Frustum
		{
			bool VSModel(const FrustumInfo& a_info, const Engine::Resource::Model* a_pModel, const DirectX::XMFLOAT4X4& a_worldMat, Result& a_outResult);
			bool VSMesh(const FrustumInfo& a_info, const Engine::Resource::Mesh* a_pMesh, const DirectX::XMFLOAT4X4& a_worldMat, Result& a_outResult);
		}
	}
}