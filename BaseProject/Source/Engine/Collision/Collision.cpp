#include "Collision.h"
#include "MidPhase/BVHTraverser/BVHTraverser.h"
#include "NarrowPhase/PrimitiveHelper/PrimitiveHelper.h"

#include "../MainEngine.h"

#include "../Graphics/RenderContext/RenderContext.h"
#include "../Graphics/RenderContext/ShapeDraw/ShapeDraw.h"

#include "../Resource/Manager/ResourceManager/ResourceManager.h"

#include "Application/Components/Collision/Collider.h"
#include "Application/Components/Collision/RayCollider.h"
#include "Application/Components/Collision/SphreCollider.h"
#include "Application/Components/Transform/LocalTransformComponent.h"
#include "Application/Components/Transform/WorldMatrixComponent.h"
#include "Application/Components/Resource/ModelComponent.h"

bool Engine::Collision::Ray::VSModel(
	const RayInfo& a_rayInfo,
	const Engine::Resource::Model* a_pModel,
	const DirectX::XMFLOAT4X4& a_worldMat,
	Result& a_outResult
)
{
	if (!a_pModel) return false;

	float _dist = 0.0f;

	// インスタンスのワールド行列をロード
	DirectX::XMMATRIX _instWorld = DirectX::XMLoadFloat4x4(&a_worldMat);

	// 判定ノードインデックスごとに処理
	for (int _idx : a_pModel->GetCollisionMeshNodeVec())
	{
		// ノード取得
		const Engine::Resource::Node& _node = a_pModel->GetOriginalNodeVec()[_idx];

		DirectX::XMMATRIX _nodeGlobal = DirectX::XMLoadFloat4x4(&_node.worldTransform);
		DirectX::XMMATRIX _combinedWorld = DirectX::XMMatrixMultiply(_nodeGlobal, _instWorld);

		DirectX::XMFLOAT4X4 _meshWorldMat;
		DirectX::XMStoreFloat4x4(&_meshWorldMat, _combinedWorld);

		for (auto& _meshIdx : _node.meshIndices)
		{
			// ノードが持つメッシュを取得
			const auto& _meshHandle = a_pModel->GetMeshHandles()[_meshIdx];
			const auto* _pMesh = Resource::ResourceManager::Instance().Get(_meshHandle);
			if (!_pMesh) continue;

			// メッシュとの判定
			if (VSMesh(a_rayInfo, _pMesh, _meshWorldMat, a_outResult))
			{
				return true;
			}
		}
	}

	return false;
}

bool Engine::Collision::Ray::VSMesh(
	const RayInfo& a_rayInfo,
	const Engine::Resource::Mesh* a_pMesh,
	const DirectX::XMFLOAT4X4& a_worldMat,
	Result& a_outResult
)
{
	// 判定を持っているかどうか
	if (!a_pMesh->HasCollisionMesh())return false;

	// 当たり判定メッシュ取得
	const auto& _collisionMesh = a_pMesh->GetCollisionMesh();

	// レイをモデル空間にするためにワールド行列の逆行列を作る
	DirectX::XMVECTOR _errVec;
	DirectX::XMMATRIX _world = DirectX::XMLoadFloat4x4(&a_worldMat);
	DirectX::XMMATRIX _invWorld = DirectX::XMMatrixInverse(&_errVec, _world);	// 逆ワールド行列
	if (DirectX::XMVectorGetX(_errVec) == 0.0f)
	{
		return false;
	}

	// レイをモデル空間に変更
	DirectX::XMVECTOR _rayOrigin = DirectX::XMLoadFloat3(&a_rayInfo.origin);
	_rayOrigin = DirectX::XMVector3TransformCoord(_rayOrigin, _invWorld);
	DirectX::XMVECTOR _direction = DirectX::XMLoadFloat3(&a_rayInfo.direction);
	_direction = DirectX::XMVector3TransformNormal(_direction, _invWorld);
	_direction = DirectX::XMVector3Normalize(_direction);

	// ローカルレイを作成
	RayInfo _localRay = {};
	DirectX::XMStoreFloat3(&_localRay.origin, _rayOrigin);
	DirectX::XMStoreFloat3(&_localRay.direction, _direction);
	_localRay.maxDistance = a_rayInfo.maxDistance;

	//----------------------------------------------------------------------------------------------------
	// BVHトラバーサルの開始
	//----------------------------------------------------------------------------------------------------
	Result _localRes = {};

	if (BVHTraverser::Traverse(_localRay, _collisionMesh, _localRes))
	{
		// ローカル空間からワールド空間へ戻す
		DirectX::XMVECTOR _hitLocal = DirectX::XMLoadFloat3(&_localRes.hitPos);
		DirectX::XMVECTOR _hitWorld = DirectX::XMVector3TransformCoord(_hitLocal, _world);
		DirectX::XMStoreFloat3(&a_outResult.hitPos, _hitWorld);

		// ワールド距離を戻す
		DirectX::XMVECTOR _rayOriginWorld = DirectX::XMLoadFloat3(&a_rayInfo.origin);
		DirectX::XMVECTOR _diff = DirectX::XMVectorSubtract(_hitWorld, _rayOriginWorld);
		float _worldDist = DirectX::XMVectorGetX(DirectX::XMVector3Length(_diff));

		a_outResult.hitDistance = _worldDist;
		a_outResult.isHit = true;
		return true;
	}
	return false;
}

// =====================================================================================
// オーバーラップ系プリミティブ（Sphere / Capsule / OBB / Frustum）の共通処理
// レイと同じ VSModel → VSMesh の流れを、ワールド空間のプリミティブを
// メッシュローカル空間へ変換してから BVH をオーバーラップ走査する形でまとめる。
// =====================================================================================
namespace Engine::Collision
{
namespace
{
	using namespace Engine::Collision::NarrowPhase;

	// ワールド空間プリミティブ → メッシュローカル空間プリミティブ

	SphereInfo TransformToLocal(const SphereInfo& a_w, DirectX::FXMMATRIX a_inv)
	{
		DirectX::BoundingSphere _ls;
		MakeSphere(a_w).Transform(_ls, a_inv);
		SphereInfo _o;
		_o.origin = _ls.Center;
		_o.radius = _ls.Radius;
		return _o;
	}

	OBBInfo TransformToLocal(const OBBInfo& a_w, DirectX::FXMMATRIX a_inv)
	{
		DirectX::BoundingOrientedBox _lb;
		MakeOBB(a_w).Transform(_lb, a_inv);
		OBBInfo _o;
		_o.center = _lb.Center;
		_o.extents = _lb.Extents;
		_o.orientation = _lb.Orientation;
		return _o;
	}

	FrustumInfo TransformToLocal(const FrustumInfo& a_w, DirectX::FXMMATRIX a_inv)
	{
		DirectX::BoundingFrustum _lf;
		MakeFrustum(a_w).Transform(_lf, a_inv);
		FrustumInfo _o;
		_o.origin = _lf.Origin;
		_o.orientation = _lf.Orientation;
		_o.rightSlope = _lf.RightSlope;
		_o.leftSlope = _lf.LeftSlope;
		_o.topSlope = _lf.TopSlope;
		_o.bottomSlope = _lf.BottomSlope;
		_o.nearPlane = _lf.Near;
		_o.farPlane = _lf.Far;
		return _o;
	}

	CapsuleInfo TransformToLocal(const CapsuleInfo& a_w, DirectX::FXMMATRIX a_inv)
	{
		DirectX::XMVECTOR _a = DirectX::XMVector3TransformCoord(DirectX::XMLoadFloat3(&a_w.pointA), a_inv);
		DirectX::XMVECTOR _b = DirectX::XMVector3TransformCoord(DirectX::XMLoadFloat3(&a_w.pointB), a_inv);

		// 半径は行列のスケール（各基底ベクトルの長さの最大）で拡縮する
		float _sx = DirectX::XMVectorGetX(DirectX::XMVector3Length(a_inv.r[0]));
		float _sy = DirectX::XMVectorGetX(DirectX::XMVector3Length(a_inv.r[1]));
		float _sz = DirectX::XMVectorGetX(DirectX::XMVector3Length(a_inv.r[2]));
		float _scale = std::max(_sx, std::max(_sy, _sz));

		CapsuleInfo _o;
		DirectX::XMStoreFloat3(&_o.pointA, _a);
		DirectX::XMStoreFloat3(&_o.pointB, _b);
		_o.radius = a_w.radius * _scale;
		return _o;
	}

	// プリミティブ vs メッシュ（オーバーラップ）
	template<typename TInfo>
	bool OverlapMesh(const TInfo& a_worldInfo, const Engine::Resource::Mesh* a_pMesh, const DirectX::XMFLOAT4X4& a_worldMat, Result& a_outResult)
	{
		if (!a_pMesh || !a_pMesh->HasCollisionMesh()) return false;
		const auto& _collisionMesh = a_pMesh->GetCollisionMesh();

		// ワールド行列の逆行列でプリミティブをメッシュローカルへ
		DirectX::XMVECTOR _errVec;
		DirectX::XMMATRIX _world = DirectX::XMLoadFloat4x4(&a_worldMat);
		DirectX::XMMATRIX _invWorld = DirectX::XMMatrixInverse(&_errVec, _world);
		if (DirectX::XMVectorGetX(_errVec) == 0.0f) return false;

		TInfo _local = TransformToLocal(a_worldInfo, _invWorld);

		Result _localRes = {};
		if (Engine::Collision::BVHTraverser::TraverseOverlap(_local, _collisionMesh, _localRes))
		{
			// ローカルの接触点をワールドへ戻す
			DirectX::XMVECTOR _hitLocal = DirectX::XMLoadFloat3(&_localRes.hitPos);
			DirectX::XMVECTOR _hitWorld = DirectX::XMVector3TransformCoord(_hitLocal, _world);
			DirectX::XMStoreFloat3(&a_outResult.hitPos, _hitWorld);
			a_outResult.hitDistance = 0.0f;
			a_outResult.isHit = true;
			return true;
		}
		return false;
	}

	// プリミティブ vs モデル（オーバーラップ）
	template<typename TInfo>
	bool OverlapModel(const TInfo& a_worldInfo, const Engine::Resource::Model* a_pModel, const DirectX::XMFLOAT4X4& a_worldMat, Result& a_outResult)
	{
		if (!a_pModel) return false;

		DirectX::XMMATRIX _instWorld = DirectX::XMLoadFloat4x4(&a_worldMat);

		for (int _idx : a_pModel->GetCollisionMeshNodeVec())
		{
			const Engine::Resource::Node& _node = a_pModel->GetOriginalNodeVec()[_idx];

			DirectX::XMMATRIX _nodeGlobal = DirectX::XMLoadFloat4x4(&_node.worldTransform);
			DirectX::XMMATRIX _combinedWorld = DirectX::XMMatrixMultiply(_nodeGlobal, _instWorld);

			DirectX::XMFLOAT4X4 _meshWorldMat;
			DirectX::XMStoreFloat4x4(&_meshWorldMat, _combinedWorld);

			for (auto& _meshIdx : _node.meshIndices)
			{
				const auto& _meshHandle = a_pModel->GetMeshHandles()[_meshIdx];
				const auto* _pMesh = Engine::Resource::ResourceManager::Instance().Get(_meshHandle);
				if (!_pMesh) continue;

				if (OverlapMesh(a_worldInfo, _pMesh, _meshWorldMat, a_outResult))
				{
					return true;
				}
			}
		}
		return false;
	}

	// 行列の最大スケール（各基底ベクトル長の最大）
	float MaxScale(DirectX::FXMMATRIX a_m)
	{
		float _sx = DirectX::XMVectorGetX(DirectX::XMVector3Length(a_m.r[0]));
		float _sy = DirectX::XMVectorGetX(DirectX::XMVector3Length(a_m.r[1]));
		float _sz = DirectX::XMVectorGetX(DirectX::XMVector3Length(a_m.r[2]));
		return std::max(_sx, std::max(_sy, _sz));
	}

	// 局所空間カプセルに対しメッシュBVHを走査し、最も深い接触を返す
	Contact CapsuleDeepestContactLocal(const CapsuleInfo& a_localCap, const Engine::Resource::CollisionMesh& a_mesh)
	{
		Contact _best;

		if (a_mesh.nodeVec.empty()) return _best;

		int _stack[64];
		int _top = 0;
		_stack[_top++] = a_mesh.rootNodeIndex;

		float _dummy = 0.0f;
		while (_top > 0)
		{
			const auto& _node = a_mesh.nodeVec[_stack[--_top]];

			if (!NarrowPhase::TestAABB(a_localCap, _node.box, _dummy)) continue;

			if (_node.IsLeaf())
			{
				for (int _i = 0; _i < _node.dataCount; ++_i)
				{
					int _triIdx = a_mesh.triangleIndiccesVec[_node.dataStart + _i];
					const auto& _tri = a_mesh.triangleVec[_triIdx];
					DXSM::Vector3 _a = _tri.v[0];
					DXSM::Vector3 _b = _tri.v[1];
					DXSM::Vector3 _c = _tri.v[2];

					Contact _ct = NarrowPhase::CapsuleTriangleContact(
						a_localCap.pointA, a_localCap.pointB, a_localCap.radius, _a, _b, _c);

					if (_ct.hit && _ct.depth > _best.depth) _best = _ct;
				}
			}
			else
			{
				if (_top < 62)
				{
					_stack[_top++] = _node.leftChild;
					_stack[_top++] = _node.rightChild;
				}
			}
		}
		return _best;
	}
}

// ---- 各プリミティブの公開関数（共通テンプレートへ委譲） ------------------------------

bool Engine::Collision::Sphere::VSModel(const SphereInfo& a_info, const Engine::Resource::Model* a_pModel, const DirectX::XMFLOAT4X4& a_worldMat, Result& a_outResult)
{
	return OverlapModel(a_info, a_pModel, a_worldMat, a_outResult);
}
bool Engine::Collision::Sphere::VSMesh(const SphereInfo& a_info, const Engine::Resource::Mesh* a_pMesh, const DirectX::XMFLOAT4X4& a_worldMat, Result& a_outResult)
{
	return OverlapMesh(a_info, a_pMesh, a_worldMat, a_outResult);
}

bool Engine::Collision::Capsule::VSModel(const CapsuleInfo& a_info, const Engine::Resource::Model* a_pModel, const DirectX::XMFLOAT4X4& a_worldMat, Result& a_outResult)
{
	return OverlapModel(a_info, a_pModel, a_worldMat, a_outResult);
}
bool Engine::Collision::Capsule::VSMesh(const CapsuleInfo& a_info, const Engine::Resource::Mesh* a_pMesh, const DirectX::XMFLOAT4X4& a_worldMat, Result& a_outResult)
{
	return OverlapMesh(a_info, a_pMesh, a_worldMat, a_outResult);
}

bool Engine::Collision::Capsule::ResolveVSMesh(const CapsuleInfo& a_info, const Engine::Resource::Mesh* a_pMesh, const DirectX::XMFLOAT4X4& a_worldMat, Contact& a_outContact)
{
	if (!a_pMesh || !a_pMesh->HasCollisionMesh()) return false;
	const auto& _collisionMesh = a_pMesh->GetCollisionMesh();

	// カプセルをメッシュローカルへ
	DirectX::XMVECTOR _errVec;
	DirectX::XMMATRIX _world = DirectX::XMLoadFloat4x4(&a_worldMat);
	DirectX::XMMATRIX _invWorld = DirectX::XMMatrixInverse(&_errVec, _world);
	if (DirectX::XMVectorGetX(_errVec) == 0.0f) return false;

	CapsuleInfo _local = TransformToLocal(a_info, _invWorld);

	// ローカル空間で最も深い接触を取得
	Contact _localContact = CapsuleDeepestContactLocal(_local, _collisionMesh);
	if (!_localContact.hit) return false;

	// 法線・深さをワールドへ戻す
	DirectX::XMVECTOR _nLocal = DirectX::XMLoadFloat3(&_localContact.normal);
	DirectX::XMVECTOR _nWorld = DirectX::XMVector3Normalize(DirectX::XMVector3TransformNormal(_nLocal, _world));

	DirectX::XMStoreFloat3(&a_outContact.normal, _nWorld);
	a_outContact.depth = _localContact.depth * MaxScale(_world);	// ローカル長→ワールド長
	a_outContact.hit = true;
	return true;
}

bool Engine::Collision::Capsule::ResolveVSModel(const CapsuleInfo& a_info, const Engine::Resource::Model* a_pModel, const DirectX::XMFLOAT4X4& a_worldMat, Contact& a_outContact)
{
	if (!a_pModel) return false;

	Contact _best;
	DirectX::XMMATRIX _instWorld = DirectX::XMLoadFloat4x4(&a_worldMat);

	for (int _idx : a_pModel->GetCollisionMeshNodeVec())
	{
		const Engine::Resource::Node& _node = a_pModel->GetOriginalNodeVec()[_idx];

		DirectX::XMMATRIX _nodeGlobal = DirectX::XMLoadFloat4x4(&_node.worldTransform);
		DirectX::XMMATRIX _combinedWorld = DirectX::XMMatrixMultiply(_nodeGlobal, _instWorld);

		DirectX::XMFLOAT4X4 _meshWorldMat;
		DirectX::XMStoreFloat4x4(&_meshWorldMat, _combinedWorld);

		for (auto& _meshIdx : _node.meshIndices)
		{
			const auto& _meshHandle = a_pModel->GetMeshHandles()[_meshIdx];
			const auto* _pMesh = Engine::Resource::ResourceManager::Instance().Get(_meshHandle);
			if (!_pMesh) continue;

			Contact _ct;
			if (ResolveVSMesh(a_info, _pMesh, _meshWorldMat, _ct))
			{
				if (_ct.depth > _best.depth) _best = _ct;
			}
		}
	}

	if (!_best.hit) return false;
	a_outContact = _best;
	return true;
}

bool Engine::Collision::OBB::VSModel(const OBBInfo& a_info, const Engine::Resource::Model* a_pModel, const DirectX::XMFLOAT4X4& a_worldMat, Result& a_outResult)
{
	return OverlapModel(a_info, a_pModel, a_worldMat, a_outResult);
}
bool Engine::Collision::OBB::VSMesh(const OBBInfo& a_info, const Engine::Resource::Mesh* a_pMesh, const DirectX::XMFLOAT4X4& a_worldMat, Result& a_outResult)
{
	return OverlapMesh(a_info, a_pMesh, a_worldMat, a_outResult);
}

bool Engine::Collision::Frustum::VSModel(const FrustumInfo& a_info, const Engine::Resource::Model* a_pModel, const DirectX::XMFLOAT4X4& a_worldMat, Result& a_outResult)
{
	return OverlapModel(a_info, a_pModel, a_worldMat, a_outResult);
}
bool Engine::Collision::Frustum::VSMesh(const FrustumInfo& a_info, const Engine::Resource::Mesh* a_pMesh, const DirectX::XMFLOAT4X4& a_worldMat, Result& a_outResult)
{
	return OverlapMesh(a_info, a_pMesh, a_worldMat, a_outResult);
}

} // namespace Engine::Collision
