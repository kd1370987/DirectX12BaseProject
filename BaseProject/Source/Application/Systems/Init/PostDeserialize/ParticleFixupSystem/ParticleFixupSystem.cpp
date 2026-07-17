#include "ParticleFixupSystem.h"

#include "Engine/ECS/World/World.h"

#include "../../../../Components/Tag/SystemPhaseTag/PostDeserializeTag.h"
#include "../../../../Components/Resource/ParticlesComponent.h"
#include "../../../../../Engine/Resource/Data/Particles/ParticlesAsset.h"

void ParticleFixupSystem::Init(Engine::ECS::World& a_world)
{
	a_world.PostDeserializeTask<ParticlesComponent>(
		Engine::ECS::ESystemType::PostDeserialize,
		"ParticleFixupSystem",
		[&a_world]
		(
			Engine::ECS::ArchetypeChunk* a_pChunk,
			uint32_t a_count,
			float a_dt,
			PostDeserializeTag* a_tag,
			ParticlesComponent* a_particleArray
			)
		{
			for (size_t _i = 0; _i < a_count; ++_i)
			{
				ParticlesComponent& _particleComp = a_particleArray[_i];

				// モデルをGUIDから取得してロードした結果のハンドルを取得
				if (_particleComp.particleGUID != Engine::DefaultGUID)
				{
					// パーティクルロード
					_particleComp.particlesAssetHandle = 
						Engine::Resource::ResourceManager::Instance().Load<Engine::Resource::ParticlesAsset>(_particleComp.particleGUID);
				}
			}
		}
	);
}
