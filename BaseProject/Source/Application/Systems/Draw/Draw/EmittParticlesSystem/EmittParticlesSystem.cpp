#include "EmittParticlesSystem.h"

#include "Engine/ECS/World/World.h"
#include "Engine/MainEngine.h"
#include "Engine/Graphics/RenderGraph/RenderGraph.h"
#include "../../../../../Engine/Graphics/GraphicEngine.h"

#include "../../../../../Engine/Resource/Data/Particles/ParticlesAsset.h"
#include "../../../../../Engine/Particle/ParticleBufferManager.h"

#include "../../../../Components/Resource/ParticlesComponent.h"
#include "../../../../Components/Transform/WorldMatrixComponent.h"

void EmittParticleSystem::Init(Engine::ECS::World& a_world)
{
	a_world.ActiveTask<const ParticlesComponent, const WorldMatrixComponent>(
		Engine::ECS::ESystemType::Draw,
		"EmittParticleSystem",
		[]
		(
			Engine::ECS::ArchetypeChunk* a_pChunk,
			uint32_t a_count,
			float a_dt,
			ActiveTag* a_tags,
			const ParticlesComponent* a_particleArray,
			const WorldMatrixComponent* a_transArray
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

				// ---------------------------------------------------------
				// ワールド行列から位置と向きを抽出
				// ---------------------------------------------------------

				// 行列の4行目(41, 42, 43)からワールド位置を抽出
				_emitData.emitPos.x = _transComp.worldMat._41;
				_emitData.emitPos.y = _transComp.worldMat._42;
				_emitData.emitPos.z = _transComp.worldMat._43;

				// 行列の3行目(31, 32, 33)から前方の向き（Z方向）を抽出
				float _dirX = _transComp.worldMat._31;
				float _dirY = _transComp.worldMat._32;
				float _dirZ = _transComp.worldMat._33;

				// スケール成分が含まれている可能性を考慮して正規化（Normalize）する
				float _length = std::sqrt(_dirX * _dirX + _dirY * _dirY + _dirZ * _dirZ);
				if (_length > 0.0001f)
				{
					_emitData.emitDirection.x = _dirX / _length;
					_emitData.emitDirection.y = _dirY / _length;
					_emitData.emitDirection.z = _dirZ / _length;
				}
				else
				{
					// 万が一スケールが0などで長さが取れなかった場合の安全策
					_emitData.emitDirection = { 0.0f, 0.0f, 1.0f };
				}

				// ---------------------------------------------------------

				_emitData.emitCount = _particleComp.particleCount;
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
				_pParticleManager->RequestEmit(_particleComp.particlesAssetHandle, _emitData);
			}
		}
	);
};
