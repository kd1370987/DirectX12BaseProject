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

#include "Engine/Collision/Gather/Gather.h"
#include "Engine/Collision/Collision.h"
#include "Engine/Collision/Query/Raycast.h"


void RayCollisionSystem::Run(World& a_world, float a_dt)
{

	// レイコライダー取得
	std::vector<RayColliderView> _rayColliderViewVec;
	Gather::GatherRayColliderViews(a_world, _rayColliderViewVec);

	// コライダー取得
	std::vector<ColliderView> _colliderViewVec;
	Gather::GatherColliderViews(a_world, _colliderViewVec);

	// レイとコライダーの当たり判定
	for (auto& _ray : _rayColliderViewVec)
	{
		auto _result = Result{};
		if (Collision::Raycast(_ray, _colliderViewVec, _result))
		{
			_ray.pTRS->pos = { 0,0.2f,0 };
		}
	};
}
