#include "RayCollisionSystem.h"

#include "Engine/ECS/World/World.h"

#include "../../Components/Collision/Collider.h"
#include "../../Components/Collision/RayCollider.h"

#include "../../Components/Transform/TRSComponent.h"
#include "../../Components/Transform/WorldMatrixComponent.h"

#include "../../Components/Resource/ModelComponent.h"
#include "Engine/ResourceManager/ResourceManager.h"

#include "Engine/GPUResource/Model/Model.h"
#include "Engine/GPUResource/Model/ModelResource/Mesh/Mesh.h"
#include "Engine/GPUResource/Model/ModelResource/Node/Node.h"

struct ColliderView
{
	ECS::Entity entity;
	TRSComponent* pTRS;
	ColliderComponent* pCollider;
	WorldMatrixComponent* pWorldMat;
};

struct RayColliderView
{
	ECS::Entity entity;
	TRSComponent* pTRS;
	RayColliderComponent* pRayCollider;
	ColliderComponent* pCollider;
	bool isHit = false;
};

void RayCollisionSystem::Run(World& a_world, float a_dt)
{

	// レイコライダー取得
	std::vector<RayColliderView> _rayColliderViewVec;
	_rayColliderViewVec.reserve(64);
	a_world.ForEach<RayColliderComponent,ColliderComponent,TRSComponent>
	(
		[&a_world, a_dt,&_rayColliderViewVec]
		(
			ArchetypeChunk* a_pChunk,
			uint32_t a_count,
			RayColliderComponent* a_rayColliderArray,
			ColliderComponent* a_colliderArray,
			TRSComponent* a_trsArray
			)
		{
			for (size_t _i = 0; _i < a_count; ++_i)
			{
				RayColliderView _view = {};
				_view.entity = a_pChunk->entityData[_i];
				_view.pRayCollider = &a_rayColliderArray[_i];
				_view.pTRS = &a_trsArray[_i];
				_view.pCollider = &a_colliderArray[_i];
				_rayColliderViewVec.push_back(_view);
			}
		}
	);

	// コライダー取得
	std::vector<ColliderView> _colliderViewVec;
	_colliderViewVec.reserve(256);
	a_world.ForEach<ColliderComponent,TRSComponent,WorldMatrixComponent>
	(
		[&a_world, a_dt,&_colliderViewVec]
		(
			ArchetypeChunk* a_pChunk,
			uint32_t a_count,
			ColliderComponent* a_colliderArray,
			TRSComponent* a_trsArray,
			WorldMatrixComponent* a_worldMatArray
			)
		{
			for (size_t _i = 0; _i < a_count; ++_i)
			{
				ColliderView _view = {};
				_view.entity = a_pChunk->entityData[_i];
				_view.pCollider = &a_colliderArray[_i];
				_view.pTRS = &a_trsArray[_i];
				_view.pWorldMat = &a_worldMatArray[_i];
				_colliderViewVec.push_back(_view);
			}
		}
	);

	// レイとコライダーの当たり判定
	for (auto& _ray : _rayColliderViewVec)
	{
		 _ray.isHit = false;

		 Collider::RayInfo _rayInfo = {};
		 _rayInfo.origin = _ray.pTRS->pos;
		 _rayInfo.direction = _ray.pRayCollider->dir;
		 _rayInfo.maxDistance = _ray.pRayCollider->length;

		for (auto& colView : _colliderViewVec)
		{
			if (_ray.entity == colView.entity)
				continue;

			if (!HasLayer(_ray.pCollider->collideLayer, colView.pCollider->layer))
				continue;

			if (colView.pCollider->layer != Layer::StaticObject)
				continue;

			ModelComponent* _modelComp = a_world.RefData<ModelComponent>(colView.entity);
			ModelResource* _modelRes = ResourceManager::Instance().NGetModelResource(_modelComp->modelID);

			for (int _idx : _modelRes->GetCollisionMeshNodeIndices())
			{
				const Node& _node = _modelRes->GetOriginalNodes()[_idx];
				if (_node.spMesh == nullptr)
					continue;

				auto* _mesh = _node.spMesh.get();
				float _nearestDist = _rayInfo.maxDistance;

				for (uint32_t _i = 0; _i < _mesh->GetFaces().size(); ++_i)
				{
					DirectX::XMMATRIX _world = DirectX::XMLoadFloat4x4(&colView.pWorldMat->worldMat);

					DirectX::XMVECTOR _v0 = DirectX::XMVector3Transform(
						DirectX::XMLoadFloat3(
							&_mesh->GetPositions()[_mesh->GetFaces()[_i].idx[0]]),_world
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
					if (Collider::RayVsMesh(_rayInfo,_v0,_v1,_v2,_dist))
					{
						_ray.isHit = true;
						_ray.pTRS->pos = {0,1,0};
						break;
					}
				}
				if(_ray.isHit)
					break;
			}

		}
	}

}
