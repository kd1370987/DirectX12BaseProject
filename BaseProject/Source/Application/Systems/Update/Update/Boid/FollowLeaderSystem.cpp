#include "FollowLeaderSystem.h"

#include "Application/ECS/World/APPWorld.h"

#include "../../../../Components/Character/BoidComponent.h"
#include "../../../../Components/Camera/FollowTargetComponent.h"
#include "../../../../Components/Transform/LocalTransformComponent.h"

//==============================================================================
// FollowLeaderSystem
//
// ボイドの目標地点(BoidComponent::targetPos)を、追従先(小隊長)の位置にする。
//
// ・PreUpdate 帯で回す。これを読む BoidSystem が PreUpdate にいるため(理由はあちらを参照)。
//   小隊長の位置は前フレームの Physics の積分結果で、Update 帯では誰も動かさないので、
//   Update に置いていたときと同じ値を読む。
//==============================================================================
void FollowLeaderSystem::Init(App::ECS::APPWorld& a_world)
{
	a_world.ActiveTask<const FollowTargetComponent,BoidComponent>(
		Engine::ECS::ESystemType::PreUpdate,
		"FollowLeaderSystem",
		[](
			Engine::ECS::Chunk* a_pChunk,
			uint32_t a_count,
			const Engine::ECS::SystemContext& a_ctx,
			ActiveTag* a_activeTags,
			const FollowTargetComponent* a_followTargetArray,
			BoidComponent* a_boidArray
		)
		{
			for (size_t _i = 0; _i < a_count; ++_i)
			{
				const FollowTargetComponent& _targetComp = a_followTargetArray[_i];
				BoidComponent& _boidComp = a_boidArray[_i];

				// 有効チェック
				if (_targetComp.target == Engine::ECS::Limits::INVALID_ENTITY) continue;
				if (!a_ctx.pWorld->HasComponent<LocalTransformComponent>(_targetComp.target)) continue;

				// リーダーの座標を取得
				LocalTransformComponent* _pTargetTrance = a_ctx.pWorld->RefData<LocalTransformComponent>(_targetComp.target);
				if (!_pTargetTrance) continue;

				// ボイドの目標地点として設定
				_boidComp.targetPos = _pTargetTrance->pos;
			}
		}
	);
}
