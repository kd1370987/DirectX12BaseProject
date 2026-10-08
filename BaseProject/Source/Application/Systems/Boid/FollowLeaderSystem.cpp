#include "FollowLeaderSystem.h"

#include "Application/ECS/World/APPWorld.h"

#include "Application/Components/Boid/BoidTargetComponent.h"
#include "Application/Components/Camera/FollowTargetComponent.h"
#include "Application/Components/Transform/LocalTransformComponent.h"

namespace App::System
{
	//==============================================================================
	// FollowLeaderSystem
	//
	// ボイドの目標地点(BoidTargetComponent)を、追従先(小隊長)の位置にする。
	//
	// ・書くのは自分の目標地点だけで、追従先の位置は読むだけなので、チャンクを分けてワーカーで回す。
	// ・PreUpdate 帯で回す。これを読む BoidSteeringSystem が PreUpdate にいるため(理由はあちらを参照)。
	//   小隊長の位置は前フレームの Physics の積分結果で、Update 帯では誰も動かさないので、
	//   Update に置いていたときと同じ値を読む。
	//==============================================================================
	void FollowLeaderSystem::Init(App::ECS::APPWorld& a_world)
	{
		a_world.ActiveJobTask<const Component::FollowTargetComponent, Component::BoidTargetComponent>(
			Engine::ECS::ESystemType::PreUpdate,
			"FollowLeaderSystem",
			[](
				Engine::ECS::Chunk* /*a_pChunk*/,
				uint32_t a_count,
				const Engine::ECS::SystemContext& a_ctx,
				Component::ActiveTag* /*a_activeTags*/,
				const Component::FollowTargetComponent* a_followTargetArray,
				Component::BoidTargetComponent* a_targetArray
			)
			{
				for (size_t _i = 0; _i < a_count; ++_i)
				{
					const Component::FollowTargetComponent& _targetComp = a_followTargetArray[_i];
					Component::BoidTargetComponent& _targetPosComp = a_targetArray[_i];

					// 有効チェック
					if (_targetComp.target == Engine::ECS::Limits::INVALID_ENTITY) continue;
					if (!a_ctx.pWorld->HasComponent<Component::LocalTransformComponent>(_targetComp.target)) continue;

					// リーダーの座標を取得
					const Component::LocalTransformComponent* _pTargetTrance = a_ctx.pWorld->RefData<Component::LocalTransformComponent>(_targetComp.target);
					if (!_pTargetTrance) continue;

					// ボイドの目標地点として設定
					_targetPosComp.targetPos = _pTargetTrance->pos;
				}
			}
		)
		// 絞り込みに使わない読み : 追従先の位置を RefData で読む
		.Reads<Component::LocalTransformComponent>();
	}
}
