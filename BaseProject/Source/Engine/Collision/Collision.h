#pragma once

struct Result;
struct ColliderView;
struct RayColliderView;

namespace Engine
{
	namespace Collision
	{
		// モデルのローカルAABB（モデル内ノードのworldTransform込み）を計算する。
		//
		// TLASへ登録するボックスは、判定側(Ray::VSModel / *::VSModel)がメッシュを置くのと
		// 同じ空間になっていないといけない。判定側は「ノードのworldTransform × インスタンス行列」を
		// 使うので、ここでもノード変換を掛けた上でマージする。
		// これを怠ると、ノードにスケールや平行移動を持つモデルでブロードフェーズが判定漏れを起こす。
		//
		// a_isMeshShape : メッシュ形状なら判定用ノード、それ以外は描画メッシュノードを対象にする
		DirectX::BoundingBox CalcModelLocalAABB(
			const Engine::Resource::Model* a_pModel,
			bool a_isMeshShape
		);

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