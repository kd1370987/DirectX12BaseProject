#include "ExplodeOnHitSystem.h"

#include "Engine/ECS/World/World.h"
#include "Engine/ECS/Internal/CollisionEvent.h"
#include "Engine/Resource/Manager/ResourceManager/ResourceManager.h"

#include "Application/Components/Collision/ExplodeOnHitComponent.h"
#include "Application/Components/Character/HealthComponent.h"
#include "Application/Utility/PrefabSpawnHelper.h"

//==============================================================================
// ExplodeOnHitSystem
//
// 何かに当たった瞬間に爆発して消えるもの(弾・ミサイルなど)を処理する。
//
// 体力を持つもの(HealthComponent)はここでは扱わない。
// 敵のように「殴られても耐える」ものまで一撃で消えてしまうため、
// 体力持ちの死亡は HealthSystem 側に一本化している。
// 撃破時に同じ爆発プレハブを出すので、見た目は変わらない。
//==============================================================================
void ExplodeOnHitSystem::Init(Engine::ECS::World& a_world)
{
	a_world.ActiveTask<const Engine::ECS::CollisionEvent, ExplodeOnHitComponent>(
		Engine::ECS::ESystemType::PostUpdate,
		"ExplodeOnHitSystem",
		[]
		(
			Engine::ECS::ArchetypeChunk* a_pChunk,
			uint32_t a_count,
			const Engine::ECS::SystemContext& a_ctx,
			ActiveTag* a_tags,
			const Engine::ECS::CollisionEvent* a_eventArray,
			ExplodeOnHitComponent* a_explodeArray
			)
		{
			for (size_t _i = 0; _i < a_count; ++_i)
			{
				const Engine::ECS::CollisionEvent& _event = a_eventArray[_i];
				ExplodeOnHitComponent& _explode = a_explodeArray[_i];

				// 未ヒットならスキップ
				if (_event.other == Engine::ECS::Limits::INVALID_ENTITY) continue;

				// ---- 爆発/エフェクトプレハブを hitPos に生成(設定されていれば) ----
				App::Utility::SpawnPrefabAt(
					*a_ctx.pWorld,
					*a_ctx.pServices->pResourceManager,
					_explode.explosionPrefabGUID,
					_explode.explosionPrefabHandle,
					_event.hitPos);

				// ---- 自分を消す ----
				if (_explode.destroySelf)
				{
					a_ctx.pWorld->AddRemoveEntity(a_pChunk->entityData[_i]);
				}
			}
		},
		// 体力を持つものは HealthSystem が面倒を見る
		Engine::ECS::Exclude<HealthComponent>()
	);
}
