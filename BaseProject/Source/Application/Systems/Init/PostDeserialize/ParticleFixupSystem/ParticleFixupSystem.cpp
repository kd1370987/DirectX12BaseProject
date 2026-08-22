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
		[]
		(
			Engine::ECS::ArchetypeChunk* a_pChunk,
			uint32_t a_count,
			const Engine::ECS::SystemContext& a_ctx,
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
					a_ctx.pServices->pResourceManager->AcquireImmediate(
						_particleComp.particlesAssetHandle, _particleComp.particleGUID);
				}

				// 火花(発動時 / 終了時)のアセットも同様にハンドルを解決しておく
				if (_particleComp.sparkGUID != Engine::DefaultGUID)
				{
					a_ctx.pServices->pResourceManager->AcquireImmediate(
						_particleComp.sparkAssetHandle, _particleComp.sparkGUID);
				}

				// 出っぱなしの指定なら、ここで再生状態にしておく。
				// isPlay は保存されないランタイム値なので、誰かが立てないと
				// ParticleEmitSystem が発生数を出さず、いつまでも出ない。
				// 状況で入り切りするもの(ブースター等)は playOnStart を false にして、
				// 制御側のシステムが毎フレーム isPlay を書く。
				_particleComp.isPlay = _particleComp.playOnStart;

				// 発生の進み具合もここでリセットしておく
				_particleComp.time = 0.0f;
				_particleComp.pendingEmitCount = 0;
				_particleComp.pendingSparkEmitCount = 0;
				_particleComp.wasPlaying = false;
			}
		}
	);
}
