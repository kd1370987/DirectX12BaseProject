#include "Collision.h"
#include "MidPhase/BVHTraverser/BVHTraverser.h"

#include "../MainEngine.h"

#include "../Graphics/RenderContext/RenderContext.h"
#include "../Graphics/RenderContext/ShapeDraw/ShapeDraw.h"

#include "../Resource/Manager/ResourceManager/ResourceManager.h"

#include "Application/Components/Collision/Collider.h"
#include "Application/Components/Collision/RayCollider.h"
#include "Application/Components/Collision/SphreCollider.h"
#include "Application/Components/Transform/TransformComponent.h"
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
