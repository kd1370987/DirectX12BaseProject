#include "ParticleEmitSystem.h"

#include "Engine/ECS/World/World.h"

#include "../../../../../Components/Resource/ParticlesComponent.h"

//==========================================================================================
// ParticleEmitSystem
//
// isPlay と emitRate から「このフレーム何個発生させるか(pendingEmitCount)」を計算する。
// 実フレーム時間(a_dt)を使うため Update フェーズで行う(Draw フェーズは dt=0 のため不可)。
// 実際の発生命令(RequestEmit)は EmittParticleSystem(Draw) が pendingEmitCount を見て行う。
//
//   emitRate > 0 : 連続発生。毎秒 emitRate 回、各回 emitCount 個(小数は time に繰り越し)。
//   emitRate == 0: バースト。isPlay の立ち上がりで一度だけ emitCount 個。
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
				ParticlesComponent& _p = a_particleArray[_i];

				// デフォルトは今フレーム発生なし
				_p.pendingEmitCount = 0;

				// 停止中はリセット
				if (!_p.isPlay)
				{
					_p.time = 0.0f;
					_p.wasPlaying = false;
					continue;
				}

				if (_p.emitRate > 0.0f)
				{
					// ---- 連続発生 : 毎秒 emitRate 回 ----
					_p.time += a_dt;
					const float _interval = 1.0f / _p.emitRate;

					int _bursts = 0;
					// 溜まった分だけ発生させ、端数は time に残す。暴走防止に上限を設ける
					while (_p.time >= _interval && _bursts < 64)
					{
						_p.time -= _interval;
						++_bursts;
					}
					_p.pendingEmitCount = _bursts * _p.emitCount;
				}
				else
				{
					// ---- バースト : isPlay の立ち上がりで一度だけ ----
					if (!_p.wasPlaying)
					{
						_p.pendingEmitCount = _p.emitCount;
					}
				}

				_p.wasPlaying = true;
			}
		}
	);
}
