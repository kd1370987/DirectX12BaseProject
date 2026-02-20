#include "Collision.h"

#include "Engine/Graphics/GraphicResource/GraphicResourceManager/GraphicResourceManager.h"
#include "Engine/Graphics/GraphicResource/Resource/Model/Model.h"
#include "Engine/Graphics/GraphicResource/Resource/Mesh/Mesh.h"
#include "Engine/Graphics/GraphicResource/Resource/Node/Node.h"

#include "Application/Components/Collision/Collider.h"
#include "Application/Components/Collision/RayCollider.h"
#include "Application/Components/Collision/SphreCollider.h"

#include "Application/Components/Transform/TRSComponent.h"
#include "Application/Components/Transform/WorldMatrixComponent.h"

#include "Application/Components/Resource/ModelComponent.h"

#include "Gather/Gather.h"
#include "Query/Raycast.h"

bool Collision::Raycast(
	const RayColliderView& a_ray, 
	const std::vector<ColliderView>& a_colliderViewVec,
	Result& a_outResult
)
{
	Collider::RayInfo _rayInfo = {};
	_rayInfo.origin = a_ray.pTRS->pos;
	_rayInfo.origin.y += a_ray.pRayCollider->pos.y; // 少し上から出す
	_rayInfo.direction = a_ray.pRayCollider->dir;
	_rayInfo.maxDistance = a_ray.pRayCollider->length;

	bool _isHit = false;

	for (auto& _colView : a_colliderViewVec)
	{
		if (a_ray.entity == _colView.entity)
			continue;

		if (!HasLayer(a_ray.pCollider->collideLayer, _colView.pCollider->layer))
			continue;

		if (_colView.pCollider->layer != Layer::StaticObject)
			continue;

		const Model* _model = GraphicResourceManager::Instance().NGetModelResource(_colView.pModelComp->modelID);

		if (!_model)
		{
			assert(0 && "モデルが取れていないためレイ判定失敗");
			return false;
		}

		for (int _idx : _model->collisionMeshNodeIndices)
		{
			const Node& _node = _model->originalNodes[_idx];
			for (auto& _meshIdx : _node.meshIndices)
			{
				auto _mesh = _model->spMeshVec[_meshIdx];
				if (!_mesh) continue;

				for (uint32_t _i = 0; _i < _mesh->GetFaces().size(); ++_i)
				{
					DirectX::XMMATRIX _world = DirectX::XMLoadFloat4x4(&_colView.pWorldMat->worldMat);

					DirectX::XMVECTOR _v0 = DirectX::XMVector3Transform(
						DirectX::XMLoadFloat3(
							&_mesh->GetPositions()[_mesh->GetFaces()[_i].idx[0]]), _world
					);
					DirectX::XMVECTOR _v1 = DirectX::XMVector3Transform(
						DirectX::XMLoadFloat3(
							&_mesh->GetPositions()[_mesh->GetFaces()[_i].idx[1]]), _world
					);
					DirectX::XMVECTOR _v2 = DirectX::XMVector3Transform(
						DirectX::XMLoadFloat3(
							&_mesh->GetPositions()[_mesh->GetFaces()[_i].idx[2]]), _world
					);

					float _dist = 0.0f;
					if (Collider::RayVsMesh(_rayInfo, _v0, _v1, _v2, _dist))
					{
						a_outResult.isHit = true;
						a_outResult.hitDistance = _dist;
						a_outResult.hitEntity = _colView.entity;
						a_outResult.hitNormal = _rayInfo.direction; // 仮
						a_outResult.hitPos.x = _rayInfo.origin.x + _rayInfo.direction.x * _dist;
						a_outResult.hitPos.y = _rayInfo.origin.y + _rayInfo.direction.y * _dist;
						a_outResult.hitPos.z = _rayInfo.origin.z + _rayInfo.direction.z * _dist;
						_isHit = true;
						break;
					}
				}
			}
			if (_isHit)
				return _isHit;
		}

	}

	return _isHit;
}
