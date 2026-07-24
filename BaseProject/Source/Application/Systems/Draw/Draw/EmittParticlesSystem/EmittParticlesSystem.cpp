#include "EmittParticlesSystem.h"

#include "Engine/ECS/World/World.h"
#include "Engine/MainEngine.h"
#include "Engine/Graphics/RenderGraph/RenderGraph.h"
#include "../../../../../Engine/Graphics/GraphicEngine.h"

#include "../../../../../Engine/Resource/Data/Particles/ParticlesAsset.h"
#include "../../../../../Engine/Particle/ParticleBufferManager.h"

#include "../../../../Components/Resource/ParticlesComponent.h"
#include "../../../../Components/Transform/WorldMatrixComponent.h"

//==========================================================================================
// EmittParticleSystem
//
// ParticleEmitSystem が計算した pendingEmitCount 個を、実際に GPU へ emit 要求する。
// 発生位置・方向は ParticlesComponent::emitSpace に従って決定し、
// スケール/拡散はコンポーネント、速度/寿命はアセットから取得する。
//==========================================================================================
void EmittParticleSystem::Init(Engine::ECS::World& a_world)
{
	a_world.ActiveTask<const ParticlesComponent, const WorldMatrixComponent>(
		Engine::ECS::ESystemType::Draw,
		"EmittParticleSystem",
		[]
		(
			Engine::ECS::ArchetypeChunk* a_pChunk,
			uint32_t a_count,
			const Engine::ECS::SystemContext& a_ctx,
			ActiveTag* a_tags,
			const ParticlesComponent* a_particleArray,
			const WorldMatrixComponent* a_transArray
			)
		{
			for (size_t _i = 0; _i < a_count; ++_i)
			{
				const ParticlesComponent& _p = a_particleArray[_i];
				const WorldMatrixComponent& _transComp = a_transArray[_i];

				// このフレームの発生数(ParticleEmitSystem が計算済み)
				if (_p.pendingEmitCount <= 0) continue;

				// パーティクルマネージャー取得
				auto* _pParticleManager = a_ctx.pServices->pMainEngine->RefParticleManager();
				if (!_pParticleManager) continue;

				// パーティクルアセット取得
				auto* _pParticle = a_ctx.pServices->pResourceManager->Get(_p.particlesAssetHandle);
				if (!_pParticle) continue;

				// ---------------------------------------------------------
				// 発生源(位置・方向)を emitSpace に応じて決定
				// ---------------------------------------------------------
				DXSM::Matrix  _world(_transComp.worldMat);
				DXSM::Vector3 _pos;
				DXSM::Vector3 _dir;

				switch (_p.emitSpace)
				{
				case EEmitSpace::WorldMatrix:
					// 付いているオブジェクトのワールド位置と前方向(+Z)
					_pos = _world.Translation();
					_dir = DXSM::Vector3(_world._31, _world._32, _world._33);
					break;

				case EEmitSpace::LocalOffset:
					// worldMat を基準に、ローカルのオフセット位置・方向を合成
					_pos = DXSM::Vector3::Transform(DXSM::Vector3(_p.posOffset), _world);
					_dir = DXSM::Vector3::TransformNormal(DXSM::Vector3(_p.emitDir), _world);
					break;

				case EEmitSpace::FixedWorld:
				default:
					// 行列を使わず、コンポーネントの絶対座標・方向をそのまま
					_pos = DXSM::Vector3(_p.worldPos);
					_dir = DXSM::Vector3(_p.emitDir);
					break;
				}

				// 方向の正規化(スケール成分や 0 ベクトルへの安全策)
				if (_dir.LengthSquared() > 1e-8f)
				{
					_dir.Normalize();
				}
				else
				{
					_dir = DXSM::Vector3(0.0f, 0.0f, 1.0f);
				}

				// ---------------------------------------------------------
				// エミットデータ構築
				// ---------------------------------------------------------
				Engine::Particle::EmitterData _emitData = {};

				_emitData.emitPos       = _pos;
				_emitData.emitDirection = _dir;
				_emitData.emitCount     = static_cast<UINT>(_p.pendingEmitCount);

				// 形状(スケール/拡散)はコンポーネントから
				_emitData.baseScale      = _p.baseScale;
				_emitData.positionRadius = _p.positionRadius;
				_emitData.directionAngle = DirectX::XMConvertToRadians(_p.directionAngle);
				_emitData.minScale       = _p.minScale;
				_emitData.maxScale       = _p.maxScale;

				// 速度・寿命はアセットから
				_emitData.minSpeed    = _pParticle->GetInitalSpeedMin();
				_emitData.maxSpeed    = _pParticle->GetInitalSpeedMax();
				_emitData.minLifeTime = _pParticle->GetLifeTimeMin();
				_emitData.maxLifeTime = _pParticle->GetLifeTimeMax();

				// 登録
				_pParticleManager->RequestEmit(_p.particlesAssetHandle, _emitData);
			}
		}
	);
};
