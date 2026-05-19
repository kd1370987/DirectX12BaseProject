#include "Collision.h"
#include "Gather/Gather.h"
#include "Query/Raycast.h"
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

bool Engine::Collision::Raycast(
	const RayColliderView& a_ray, 
	const std::vector<ColliderView>& a_colliderViewVec,
	Result& a_outResult
)
{
	// レイ情報作成
	RayInfo _rayInfo = {};
	_rayInfo.origin = a_ray.pTRS->pos;
	_rayInfo.origin.y += a_ray.pRayCollider->pos.y; // 少し上から出す
	_rayInfo.direction = a_ray.pRayCollider->dir;
	_rayInfo.maxDistance = a_ray.pRayCollider->length;
	// 判定
	bool _isHit = false;
	float _dist = 100.0f;
	for (auto& _colView : a_colliderViewVec)
	{
		// 自分は無視
		if (a_ray.entity == _colView.entity)
			continue;

		// レイヤーが一致しているかどうか
		if (!HasLayer(a_ray.pCollider->collideLayer, _colView.pCollider->layer))
			continue;

		if (_colView.pCollider->layer != Layer::StaticObject)
			continue;

		// モデル取得
		auto* _model = Engine::Resource::ResourceManager::Instance().Get(_colView.pModelComp->handle);
		if (!_model)
		{
			assert(0 && "モデルが取れていないためレイ判定失敗");
			return false;
		}

		// モデルと判定
		if (Engine::Collision::Ray::VSModel(_rayInfo,_model, _colView.pWorldMat->worldMat,a_outResult))
		{
			_isHit = true;
			if (a_outResult.hitDistance < _dist)
			{
				a_outResult.hitEntity = _colView.entity;
				a_outResult.hitNormal = _rayInfo.direction; // 仮
				_dist = a_outResult.hitDistance;
			}		
		}
	}

	return _isHit;
}

bool Engine::Collision::Ray::VSModel(
	const RayInfo& a_rayInfo,
	const Engine::Resource::Model* a_pModel,
	const DirectX::XMFLOAT4X4& a_worldMat,
	Result& a_outResult
)
{
	if (!a_pModel) return false;

	float _dist = 0.0f;

	// 判定ノードインデックスごとに処理
	for (int _idx : a_pModel->GetCollisionMeshNodeVec())
	{
		// ノード取得
		const Engine::Resource::Node& _node = a_pModel->GetOriginalNodeVec()[_idx];
		for (auto& _meshIdx : _node.meshIndices)
		{
			// ノードが持つメッシュを取得
			auto _pMesh = a_pModel->GetSPMeshVec()[_meshIdx].get();
			if (!_pMesh) continue;

			// メッシュとの判定
			if (VSMesh(a_rayInfo, _pMesh,a_worldMat, a_outResult))
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
