#include "BoidGroundEffectSystem.h"

#include "Application/ECS/World/APPWorld.h"

#include "../../../../Components/Character/Boss/WarmGroundEffectComponent.h"
#include "../../../../Components/Transform/LocalTransformComponent.h"
#include "../../../../Components/Collision/Collider.h"
#include "../../../../InstanceResource/WormGroundEffectResource.h"
#include "../../../../Utility/EffectSpawnHelper.h"

#include "Engine/Physics/PhysicsWorld.h"

//==============================================================================
// BoidGroundEffectSystem
//
// ワームの体(ボイド)から上下にレイを打ち、地表へ砂埃を炊く。
// どのエフェクトをどう出すかは WormGroundEffectResource(SwarmBossController が書く)。
//
//   地面の中 … 真上へ打ったレイが上向きの面の裏に当たった = 地表の下。
//              深さに関係なく、当たった地表へ underScale で炊く
//   地面の上 … 真下へ maxHeight だけ打ち、当たれば地表へ炊く。
//              近いほど大きく(nearScale)、maxHeight ぎりぎりで farScale
//
// ・レイを打つのは残り時間(WarmGroundEffectComponent.timer)が切れたボイドだけ。
//   間隔 1 秒なら 4000 体でも 1 フレームに 70 体ほど(レイは最大でその2倍)。
// ・1 フレームに出す数が上限に達したら、そのフレームは打たずに時間だけ戻す(炊き損ねは捨てる)。
//   待たせると、先に並んだチャンクのボイドばかりが毎回炊くことになるため。
// ・地面として見るのは StaticObject だけ(SerchGroundSystem と同じ)。
// ・エフェクトは遅延生成(SpawnEffectAt)。出し切ったら自分から消える。
//==============================================================================
void BoidGroundEffectSystem::Init(App::ECS::APPWorld& a_world)
{
	a_world.ActiveTask<WarmGroundEffectComponent, const LocalTransformComponent>(
		Engine::ECS::ESystemType::Update,
		"BoidGroundEffectSystem",
		[](
			Engine::ECS::Chunk*               a_pChunk,
			uint32_t                          a_count,
			const Engine::ECS::SystemContext& a_ctx,
			ActiveTag*                        a_tags,
			WarmGroundEffectComponent*        a_effectArray,
			const LocalTransformComponent*    a_transArray
		)
		{
			if (!a_ctx.pWorld->HasResource<WormGroundEffectResource>()) return;
			auto& _res = a_ctx.pWorld->GetResource<WormGroundEffectResource>();
			if (!_res.isActive || _res.effectGUID == Engine::DefaultGUID) return;

			ENGINE_PROFILE_SCOPE("Physics_BoidGroundEffectRay");
			const auto& _physicsWorld = a_ctx.pWorld->GetResource<Engine::Physics::PhysicsWorld>();

			const float _dt = a_ctx.dt;
			const uint32_t _queryMask = static_cast<uint32_t>(Layer::StaticObject);
			const float _interval = std::max(_res.interval, 0.01f);

			for (size_t _i = 0; _i < a_count; ++_i)
			{
				WarmGroundEffectComponent& _effect = a_effectArray[_i];

				_effect.timer -= _dt;
				if (_effect.timer > 0.0f) continue;

				// 次の番。毎回ばらして、同じフレームに揃ったものも次第にずれていくようにする
				_effect.timer = _interval * Math::Random::Float(0.75f, 1.25f);

				// 今フレームはもう出せない。レイも打たない
				if (_res.spawnedThisFrame >= _res.maxSpawnPerFrame) continue;

				const Engine::ECS::Entity _self = a_pChunk->entityData[_i];
				const Math::Vector3 _pos = a_transArray[_i].pos;

				Math::Vector3 _spawnPos = {};
				Math::Vector3 _emitDir  = {};
				float _scale = 0.0f;
				bool _isSpawn = false;

				//------------------------------------------------------
				// 地面の中 : 真上の地表へ常に炊く
				//------------------------------------------------------
				Math::Ray _upRay;
				_upRay.origin      = _pos;
				_upRay.direction   = { 0.0f, 1.0f, 0.0f };
				_upRay.maxDistance = _res.maxDepth;

				Engine::Physics::RayHit _upHit = {};
				if (_physicsWorld.CastRay(_upRay, _queryMask, _self, _upHit) && _upHit.normal.y > 0.0f)
				{
					_spawnPos = _upHit.position;
					_emitDir  = _upHit.normal;
					_scale    = _res.underScale;
					_isSpawn  = true;
				}
				//------------------------------------------------------
				// 地面の上 : 真下の地面が近ければ、近いほど大きく炊く
				//------------------------------------------------------
				else if (_res.maxHeight > 0.0f)
				{
					Math::Ray _downRay;
					_downRay.origin      = _pos;
					_downRay.direction   = { 0.0f, -1.0f, 0.0f };
					_downRay.maxDistance = _res.maxHeight;

					Engine::Physics::RayHit _downHit = {};
					if (_physicsWorld.CastRay(_downRay, _queryMask, _self, _downHit))
					{
						const float _t = std::clamp(_downHit.distance / _res.maxHeight, 0.0f, 1.0f);

						_spawnPos = _downHit.position;
						_emitDir  = _downHit.normal;
						_scale    = _res.nearScale + (_res.farScale - _res.nearScale) * _t;
						_isSpawn  = true;
					}
				}

				if (!_isSpawn || _scale <= 0.0f) continue;

				if (App::Utility::SpawnEffectAt(*a_ctx.pWorld, _res.effectGUID, _spawnPos, true, _emitDir, _scale))
				{
					++_res.spawnedThisFrame;
				}
			}
		}
	);
}
