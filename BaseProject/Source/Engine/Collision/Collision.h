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

			// 押し出し用：カプセルとメッシュ/モデルの「最も深い接触」をワールド空間で返す
			bool ResolveVSMesh(const CapsuleInfo& a_info, const Engine::Resource::Mesh* a_pMesh, const DirectX::XMFLOAT4X4& a_worldMat, Contact& a_outContact);
			bool ResolveVSModel(const CapsuleInfo& a_info, const Engine::Resource::Model* a_pModel, const DirectX::XMFLOAT4X4& a_worldMat, Contact& a_outContact);
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