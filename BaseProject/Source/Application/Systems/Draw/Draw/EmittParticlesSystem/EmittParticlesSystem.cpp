#include "EmittParticlesSystem.h"

#include "Engine/ECS/World/World.h"
#include "Engine/MainEngine.h"
#include "Engine/Graphics/RenderGraph/RenderGraph.h"
#include "../../../../../Engine/Graphics/GraphicEngine.h"

#include "../../../../../Engine/Resource/Data/Particles/ParticlesAsset.h"
#include "../../../../../Engine/Particle/ParticleBufferManager.h"

#include "../../../../Components/Resource/ParticlesComponent.h"
#include "../../../../Components/Transform/TransformComponent.h"

void EmittParticleSystem::Init(Engine::ECS::World& a_world)
{
	a_world.ActiveTask<const ParticlesComponent, const TransformComponent>(
		Engine::ECS::ESystemType::Draw,
		"EmittParticleSystem",
		[]
		(
			Engine::ECS::ArchetypeChunk* a_pChunk,
			uint32_t a_count,
			float a_dt,
			ActiveTag* a_tags,
			const ParticlesComponent* a_particleArray,
			const TransformComponent* a_transArray
			)
		{
			for (size_t _i = 0; _i < a_count; ++_i)
			{
				const auto& _particleComp = a_particleArray[_i];
				const auto& _transComp = a_transArray[_i];

				auto* _pParticleManager = Engine::MainEngine::Instance().RefParticleManager();
				if (!_pParticleManager) continue;

				// エミットデータ : とりあえず上方向に100個
				Engine::Particle::EmitterData _emitData = {};
				_emitData.emitPos = _transComp.pos;
				_emitData.emitCount = 100;
				_emitData.emitDirection = { 0,1,0 };

				// 登録
				_pParticleManager->RequestEmit(_particleComp.particlesAssetHandle,_emitData);
			}
		}
	);
};
