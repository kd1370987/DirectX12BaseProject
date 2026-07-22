#include "ParticleEmitSystem.h"

#include "Engine/ECS/World/World.h"

#include "../../../../../Components/Resource/ParticlesComponent.h"

//==========================================================================================
// ParticleEmitSystem
//
// パーティクルの再生状態(isPlay)を進める。
// isPlay の ON/OFF 自体は、アタッチメントのブースターであれば
// AttachmentDispatchSystem がスロット経由で設定する
// (それ以外の発生源は各々のロジックで isPlay を立てる)。
// ここでは再生中の経過時間だけを進め、実際の噴射は EmittParticleSystem が行う。
//==========================================================================================
void ParticleEmitSystem::Init(Engine::ECS::World& a_world)
{
	a_world.ActiveTask<ParticlesComponent>(
		Engine::ECS::ESystemType::Update,
		"ParticleEmitSystem",
		[]
		(
			Engine::ECS::ArchetypeChunk* a_pChunk,
			uint32_t a_count,
			float a_dt,
			ActiveTag* a_tags,
			ParticlesComponent* a_particleArray
			)
		{
			for (size_t _i = 0; _i < a_count; ++_i)
			{
				ParticlesComponent& _particleComp = a_particleArray[_i];

				// 再生中は経過時間を進め、停止したらリセット
				if (_particleComp.isPlay)
				{
					_particleComp.time += a_dt;
				}
				else
				{
					_particleComp.time = 0.0f;
				}
			}
		}
	);
}
