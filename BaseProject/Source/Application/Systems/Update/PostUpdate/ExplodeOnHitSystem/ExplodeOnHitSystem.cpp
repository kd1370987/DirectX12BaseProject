#include "ExplodeOnHitSystem.h"

#include <cstring>

#include "Engine/ECS/World/World.h"
#include "Engine/ECS/Internal/CollisionEvent.h"
#include "Engine/Resource/Manager/ResourceManager/ResourceManager.h"
#include "Engine/Resource/Data/Prefab/Prefab.h"

#include "Application/Components/Collision/ExplodeOnHitComponent.h"
#include "Application/Components/Transform/LocalTransformComponent.h"

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
				if (_explode.explosionPrefabGUID != Engine::DefaultGUID)
				{
					auto& _rm = *a_ctx.pServices->pResourceManager;
					if (!_rm.IsValid(_explode.explosionPrefabHandle))
					{
						if (!_rm.Has<Engine::Resource::Prefab>(_explode.explosionPrefabGUID))
						{
							_rm.Load<Engine::Resource::Prefab>(_explode.explosionPrefabGUID);
						}
						_explode.explosionPrefabHandle = _rm.GetCache<Engine::Resource::Prefab>(_explode.explosionPrefabGUID);
					}

					auto* _pPrefab = _rm.Ref(_explode.explosionPrefabHandle);
					if (_pPrefab)
					{
						Engine::ECS::Signature _sig = _pPrefab->GetSignature();
						auto _data = _pPrefab->GetDataMap();	// コピー

						auto _ltID = a_ctx.pWorld->GetCompTypeID<LocalTransformComponent>();

						// 位置を入れるため LocalTransform が無ければ足す
						if (!_sig.test(_ltID))
						{
							_sig.set(_ltID);
							auto& _buf = _data[_ltID];
							_buf.assign(a_ctx.pWorld->GetComponentMetaData(_ltID).compAlignSize, 0);
							auto _ctor = a_ctx.pWorld->GetCompFunc(_ltID).construct;
							if (_ctor) _ctor(_buf.data());
						}

						// hitPos に配置
						{
							auto& _buf = _data[_ltID];
							LocalTransformComponent _lt = {};
							std::memcpy(&_lt, _buf.data(), sizeof(_lt));
							_lt.pos = _event.hitPos;
							_lt.isDirty = true;
							std::memcpy(_buf.data(), &_lt, sizeof(_lt));
						}

						// 反復中なので遅延生成
						a_ctx.pWorld->AddEntityWithData(_sig, std::move(_data));
					}
				}

				// ---- 自分を消す ----
				if (_explode.destroySelf)
				{
					a_ctx.pWorld->AddRemoveEntity(a_pChunk->entityData[_i]);
				}
			}
		}
	);
}
