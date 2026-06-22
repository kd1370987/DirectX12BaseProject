#include "ParticleEmitSystem.h"

#include "Engine/ECS/World/World.h"
#include "../../../../../../Engine/MainEngine.h"
#include "../../../../../../Engine/Particle/ParticleBufferManager.h"

#include "../../../../../Components/Resource/ParticlesComponent.h"
#include "Application/Components/Transform/TransformComponent.h"


void ParticleEmitSystem::Init(Engine::ECS::World& a_world)
{
	a_world.ActiveTask<const TransformComponent, ParticlesComponent>(
		Engine::ECS::ESystemType::Update,
		"ParticleEmitSystem",
		[]
		(
			Engine::ECS::ArchetypeChunk* a_pChunk,
			uint32_t a_count,
			float a_dt,
			ActiveTag* a_tags,
			const TransformComponent* a_trsArray,
			ParticlesComponent* a_particleArray
			)
		{
			bool _isTest = Engine::Input::InputManager::Instance().IsPress("Test");


			for (size_t _i = 0; _i < a_count; ++_i)
			{
				// 本来はパーティクルコンポーネントから引っ張ってくるがいったんテストで固定値
				ParticlesComponent& _particleComp = a_particleArray[_i];
				const TransformComponent& _trsComp = a_trsArray[_i];

				if (_isTest)
				{
					_particleComp.isPlay = true;
				}
				else
				{
					_particleComp.isPlay = false;
				}
			}
		}
	);
}
