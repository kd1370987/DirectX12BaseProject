#include "Collision.h"
#include "Engine/Resource/Manager/ModelManager/ModelManager.h"

#include "Application/Components/Collision/Collider.h"
#include "Application/Components/Collision/RayCollider.h"
#include "Application/Components/Collision/SphreCollider.h"

#include "Application/Components/Transform/TransformComponent.h"
#include "Application/Components/Transform/WorldMatrixComponent.h"

#include "Application/Components/Resource/ModelComponent.h"

#include "Gather/Gather.h"
#include "Query/Raycast.h"

bool Engine::Collision::Raycast(
	const RayColliderView& a_ray, 
	const std::vector<ColliderView>& a_colliderViewVec,
	Result& a_outResult
)
{
	// レイ情報作成
	Collider::RayInfo _rayInfo = {};
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
		auto* _model = Engine::Resource::ModelManager::Instnace().GetModel(_colView.pModelComp->handle);
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

		// 判定ノードインデックスごとに処理
		//for (int _idx : _model->collisionMeshNodeIndices)
		//{
		//	// ノード取得
		//	const Node& _node = _model->originalNodes[_idx];
		//	for (auto& _meshIdx : _node.meshIndices)
		//	{
		//		// ノードが持つメッシュを取得
		//		auto _mesh = _model->spMeshVec[_meshIdx];
		//		if (!_mesh) continue;

		//		for (uint32_t _i = 0; _i < _mesh->GetFaces().size(); ++_i)
		//		{
		//			DirectX::XMMATRIX _world = DirectX::XMLoadFloat4x4(&_colView.pWorldMat->worldMat);

		//			DirectX::XMVECTOR _v0 = DirectX::XMVector3Transform(
		//				DirectX::XMLoadFloat3(
		//					&_mesh->GetPositions()[_mesh->GetFaces()[_i].idx[0]]), _world
		//			);
		//			DirectX::XMVECTOR _v1 = DirectX::XMVector3Transform(
		//				DirectX::XMLoadFloat3(
		//					&_mesh->GetPositions()[_mesh->GetFaces()[_i].idx[1]]), _world
		//			);
		//			DirectX::XMVECTOR _v2 = DirectX::XMVector3Transform(
		//				DirectX::XMLoadFloat3(
		//					&_mesh->GetPositions()[_mesh->GetFaces()[_i].idx[2]]), _world
		//			);

		//			float _dist = 0.0f;
		//			if (Collider::RayVsMesh(_rayInfo, _v0, _v1, _v2, _dist))
		//			{
		//				a_outResult.isHit = true;
		//				a_outResult.hitDistance = _dist;
		//				a_outResult.hitEntity = _colView.entity;
		//				a_outResult.hitNormal = _rayInfo.direction; // 仮
		//				a_outResult.hitPos.x = _rayInfo.origin.x + _rayInfo.direction.x * _dist;
		//				a_outResult.hitPos.y = _rayInfo.origin.y + _rayInfo.direction.y * _dist;
		//				a_outResult.hitPos.z = _rayInfo.origin.z + _rayInfo.direction.z * _dist;
		//				_isHit = true;
		//				break;
		//			}
		//		}
		//	}
		//	if (_isHit)
		//		return _isHit;
		//}

	}

	return _isHit;
}

bool Engine::Collision::Ray::VSModel(
	const Collider::RayInfo& a_rayInfo,
	const Engine::Resource::Model* a_pModel,
	const DirectX::XMFLOAT4X4& a_worldMat,
	Result& a_outResult
)
{
	if (!a_pModel) return false;

	float _dist = 0.0f;

	// 判定ノードインデックスごとに処理
	//for (int _idx : a_pModel->collisionMeshNodeIndices)
	for (int _idx : a_pModel->GetCollisionMeshNodeVec())
	{
		// ノード取得
		//const Engine::Resource::Node& _node = a_pModel->originalNodes[_idx];
		const Engine::Resource::Node& _node = a_pModel->GetOriginalNodeVec()[_idx];
		for (auto& _meshIdx : _node.meshIndices)
		{
			// ノードが持つメッシュを取得
			//auto _spMesh = a_pModel->spMeshVec[_meshIdx];
			auto _spMesh = a_pModel->GetSPMeshVec()[_meshIdx];
			if (!_spMesh) continue;

			// メッシュとの判定
			if (VSMesh(a_rayInfo, _spMesh.get(),a_worldMat, a_outResult))
			{
				return true;
			}
		}
	}

	return false;
}

bool Engine::Collision::Ray::VSMesh(
	const Collider::RayInfo& a_rayInfo, 
	const Engine::Resource::Mesh* a_pMesh,
	const DirectX::XMFLOAT4X4& a_worldMat,
	Result& a_outResult
)
{
	// 判定を持っているかどうか
	if (!a_pMesh->HasCollision())return false;

	// 当たり判定メッシュ取得
	const Engine::Collision::Mesh& _collisionMesh = a_pMesh->GetCollision();

	// レイをモデル空間にするためにワールド行列の逆行列を作る
	DirectX::XMVECTOR _errVec;
	DirectX::XMMATRIX _world = DirectX::XMLoadFloat4x4(&a_worldMat);
	DirectX::XMMATRIX _invWorld = DirectX::XMMatrixInverse(&_errVec,_world);			// 逆ワールド行列
	DirectX::XMMATRIX _invTrans = DirectX::XMMatrixTranspose(_invWorld);			// 逆行列座標
	if (DirectX::XMVectorGetX(_errVec) == 0.0f)
	{
		return false;
	}



	// レイをモデル空間に変更
	DirectX::XMVECTOR _rayOrigin = DirectX::XMLoadFloat3(&a_rayInfo.origin);
	_rayOrigin = DirectX::XMVector3TransformCoord(_rayOrigin,_invWorld);
	DirectX::XMVECTOR _direction = DirectX::XMLoadFloat3(&a_rayInfo.direction);
	_direction = DirectX::XMVector3TransformNormal(_direction,_invWorld);
	_direction = DirectX::XMVector3Normalize(_direction);

	// ローカルレイを作成
	Collider::RayInfo _localRay = {};
	DirectX::XMStoreFloat3(&_localRay.origin ,_rayOrigin);
	DirectX::XMStoreFloat3(&_localRay.direction,_direction);
	_localRay.maxDistance = a_rayInfo.maxDistance;


	// ローカル結果
	Result _localRes = {};

	// メッシュ全体と当たっているかどうか
	bool _isHit = false;
	float _dist = 0.0f;
	if (_collisionMesh.localAABB.Intersects(_rayOrigin, _direction, _dist))
	{
		// メッシュのグリッドと判定
		for (auto& _cell : _collisionMesh.grid.cellVec)
		{
			// セルボックスと判定
			if (_cell.box.Intersects(_rayOrigin, _direction, _dist))
			{
				// セル内のポリゴンとの判定
				for (auto& _idx : _cell.triangleVec)
				{
					auto& _triangle = _collisionMesh.triangleVec[_idx];
					if (_cell.box.Intersects(_rayOrigin, _direction, _dist))
					{
						// ポリゴンボックスとヒットしたら、ポリゴンと判定
						DirectX::XMVECTOR _vec0 = DirectX::XMLoadFloat3(&_triangle.v[0]);
						DirectX::XMVECTOR _vec1 = DirectX::XMLoadFloat3(&_triangle.v[1]);
						DirectX::XMVECTOR _vec2 = DirectX::XMLoadFloat3(&_triangle.v[2]);
						if (Collider::RayVsMesh(_localRay, _vec0, _vec1, _vec2, _dist))
						{
							// ポリゴンとヒットしていたらデータを入れる
							if (_dist > _localRes.hitDistance)
							{
								_localRes.isHit = true;
								_localRes.hitDistance = _dist;
								_localRes.hitPos.x = _localRay.origin.x + _localRay.direction.x * _dist;
								_localRes.hitPos.y = _localRay.origin.y + _localRay.direction.y * _dist;
								_localRes.hitPos.z = _localRay.origin.z + _localRay.direction.z * _dist;
							}
							_isHit = true;
						}
					}
				}
			}
			if (_isHit)
			{
				// ローカル空間からワールド空間へ戻す
				DirectX::XMVECTOR _hitLocal = DirectX::XMLoadFloat3(&_localRes.hitPos);
				DirectX::XMVECTOR _hitWorld = DirectX::XMVector3TransformCoord(_hitLocal,_world);
				DirectX::XMStoreFloat3(&a_outResult.hitPos, _hitWorld);

				// ワールド距離を戻す
				DirectX::XMVECTOR _rayOriginWorld = DirectX::XMLoadFloat3(&a_rayInfo.origin);
				DirectX::XMVECTOR _diff = DirectX::XMVectorSubtract(_hitWorld,_rayOriginWorld);
				float _worldDist = DirectX::XMVectorGetX(DirectX::XMVector3Length(_diff));

				a_outResult.hitDistance = _worldDist;
				a_outResult.isHit = true;
				return true;
			}
		}
	}
	
	return false;
}
