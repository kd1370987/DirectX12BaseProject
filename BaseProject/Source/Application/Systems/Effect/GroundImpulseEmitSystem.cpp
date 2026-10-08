#include "GroundImpulseEmitSystem.h"

#include "Engine/MainEngine.h"
#include "Engine/Graphics/GraphicsEngine.h"

#include "Application/ECS/World/APPWorld.h"

#include "Application/Components/Effect/GroundImpulseEmitterComponent.h"
#include "Application/Components/Transform/WorldMatrixComponent.h"
#include "Engine/Graphics/Frame/SceneView/GroundImpulse.h"

namespace App::System
{
	//==============================================================================
	// GroundImpulseEmitSystem
	//
	// GroundImpulseEmitterComponent を持つエンティティが、出現した瞬間に衝撃を1発出す。
	// 衝撃は SceneView に毎フレーム積み直す必要があるので(EndFrame で空になる)、
	// lifetime を過ぎるまで経過時間を進めながら積み続ける。
	//
	// ・エディターのボタン(Fire)は age を 0 へ戻すだけ。出し直しはここが拾う。
	// ・PostUpdate 帯 : エディターで止めている間も dt が進むので、ボタンでも波が広がる
	//==============================================================================
	void GroundImpulseEmitSystem::Init(App::ECS::APPWorld& a_world)
	{
		a_world.ActiveTask<Component::GroundImpulseEmitterComponent, const Component::WorldMatrixComponent>(
			Engine::ECS::ESystemType::PostUpdate,
			"GroundImpulseEmitSystem",
			[]
			(
				Engine::ECS::Chunk*                 a_pChunk,
				uint32_t                            a_count,
				const Engine::ECS::SystemContext&   a_ctx,
				Component::ActiveTag*                          a_tags,
				Component::GroundImpulseEmitterComponent*      a_emitterArray,
				const Component::WorldMatrixComponent*         a_worldMatArray
			)
			{
				auto* _pGE = a_ctx.pServices->pMainEngine->RefGraphicsEngine();
				if (!_pGE) return;

				auto* _pSceneView = _pGE->RefSceneView();
				if (!_pSceneView) return;

				for (size_t _i = 0; _i < a_count; ++_i)
				{
					Component::GroundImpulseEmitterComponent& _emitter = a_emitterArray[_i];

					// 出現した瞬間の1発
					if (!_emitter.isFired)
					{
						_emitter.isFired = true;
						_emitter.age = 0.0f;
					}

					// 出していない / 出し終えた
					if (_emitter.age < 0.0f) continue;
					if (_emitter.age > _emitter.lifetime)
					{
						_emitter.age = -1.0f;
						continue;
					}

					Engine::Graphics::GroundImpulse _impulse = {};
					_impulse.pos = Math::Matrix(a_worldMatArray[_i].worldMat).Translation();
					_impulse.radius = _emitter.radius;
					_impulse.strength = _emitter.strength;
					_impulse.speed = _emitter.speed;
					_impulse.width = _emitter.width;
					_impulse.lifetime = _emitter.lifetime;
					_impulse.age = _emitter.age;
					_pSceneView->AddGroundImpulse(_impulse);

					_emitter.age += a_ctx.dt;
				}
			}
		);
	}
}
