#include "RayCollisionSystem.h"

#include "Engine/ECS/World/World.h"

#include "Application/Components/Collision/Collider.h"
#include "Application/Components/Collision/RayCollider.h"
#include "Application/Components/Transform/TRSComponent.h"
#include "Application/Components/Transform/WorldMatrixComponent.h"
#include "Application/Components/Resource/ModelComponent.h"

#include "Engine/Graphics/GraphicResource/GraphicResourceManager/GraphicResourceManager.h"

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
		if (Engine::Collision::Raycast(_ray, _colliderViewVec, _result))
		{
			_ray.pTRS->pos = _result.hitPos;
		}
	};
}
