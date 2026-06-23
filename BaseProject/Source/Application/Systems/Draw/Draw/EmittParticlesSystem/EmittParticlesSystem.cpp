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

				// パーティクル発射命令がオフなら処理しない
				if (!_particleComp.isPlay) continue;

				// パーティクルマネージャー取得
				auto* _pParticleManager = Engine::MainEngine::Instance().RefParticleManager();
				if (!_pParticleManager) continue;

				// パーティクルアセット取得
				auto* _pParticle = Engine::Resource::ResourceManager::Instance().Get(_particleComp.particlesAssetHandle);
				if (!_pParticle) continue;

				// エミットデータ
				Engine::Particle::EmitterData _emitData = {};
				_emitData.emitPos = _transComp.pos;
				_emitData.emitCount = Math::Random::Int(800, 1000);
				_emitData.emitDirection = DXSM::Vector3(0,1,0);
				_emitData.baseScale = 1;

				_emitData.positionRadius = 0.5f;
				_emitData.directionAngle = DirectX::XMConvertToRadians(10);

				_emitData.minScale = 0.1f;
				_emitData.maxScale = 1.0f;

				_emitData.minSpeed = _pParticle->GetInitalSpeedMin();
				_emitData.maxSpeed = _pParticle->GetInitalSpeedMax();

				_emitData.minLifeTime = _pParticle->GetLifeTimeMin();
				_emitData.maxLifeTime = _pParticle->GetLifeTimeMax();


				// 登録
				_pParticleManager->RequestEmit(_particleComp.particlesAssetHandle,_emitData);
			}
		}
	);
};
