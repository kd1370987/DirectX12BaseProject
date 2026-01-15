#include "Gather.h"

#include "Engine/ECS/World/World.h"

#include "Application/Components/Collision/Collider.h"
#include "Application/Components/Collision/RayCollider.h"
#include "Application/Components/Collision/SphreCollider.h"

#include "Application/Components/Transform/TRSComponent.h"
#include "Application/Components/Transform/WorldMatrixComponent.h"

#include "Application/Components/Resource/ModelComponent.h"

void Gather::GatherColliderViews(World& a_world, std::vector<ColliderView>& a_outColliderViewVec)
{
	a_outColliderViewVec.reserve(256);
	a_world.ForEach<ColliderComponent, TRSComponent, WorldMatrixComponent,ModelComponent>
		(
			[&a_world,&a_outColliderViewVec]
			(
				ArchetypeChunk* a_pChunk,
				uint32_t a_count,
				ColliderComponent* a_colliderArray,
				TRSComponent* a_trsArray,
				WorldMatrixComponent* a_worldMatArray,
				ModelComponent* a_modelCompArray
				)
			{
				for (size_t _i = 0; _i < a_count; ++_i)
				{
					ColliderView _view = {};
					_view.entity = a_pChunk->entityData[_i];
					_view.pCollider = &a_colliderArray[_i];
					_view.pTRS = &a_trsArray[_i];
					_view.pWorldMat = &a_worldMatArray[_i];
					_view.pModelComp = &a_modelCompArray[_i];
					a_outColliderViewVec.push_back(_view);
				}
			}
		);
}

void Gather::GatherRayColliderViews(World& a_world, std::vector<RayColliderView>& a_outRayColliderViewVec)
{
	a_outRayColliderViewVec.reserve(64);
	a_world.ForEach<RayColliderComponent, ColliderComponent, TRSComponent>
		(
			[&a_world,&a_outRayColliderViewVec]
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
					a_outRayColliderViewVec.push_back(_view);
				}
			}
		);
}
