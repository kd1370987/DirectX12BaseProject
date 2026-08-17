#include "EmitParticlesSystem.h"

#include "Engine/ECS/World/World.h"
#include "Engine/MainEngine.h"
#include "Engine/Graphics/RenderGraph/RenderGraph.h"
#include "../../../../../Engine/Graphics/GraphicEngine.h"

#include "../../../../../Engine/Resource/Data/Particles/ParticlesAsset.h"
#include "../../../../../Engine/Particle/ParticleBufferManager.h"

#include "../../../../Components/Resource/ParticlesComponent.h"
#include "../../../../Components/Transform/WorldMatrixComponent.h"
#include "../../../../Components/Force/VelocityComponent.h"

//==========================================================================================
// EmitParticleSystem
//
// ParticleEmitSystem が計算した pendingEmitCount 個を、実際に GPU へ emit 要求する。
// 発生位置・方向は ParticlesComponent::emitSpace に従って決定し、
// スケール/拡散はコンポーネント、速度/寿命はアセットから取得する。
//
// 火花(pendingSparkEmitCount)がある場合は、同じ発生源から別アセットを
// もう1回 emit する。点火/消火のフレームは本体と火花が同時に出る。
//==========================================================================================
void EmitParticleSystem::Init(Engine::ECS::World& a_world)
{
	a_world.ActiveTask<const ParticlesComponent, const WorldMatrixComponent>(
		Engine::ECS::ESystemType::Draw,
		"EmitParticleSystem",
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
				// 火花は本体が止まったフレームにも出るので、どちらかがあれば処理する
				if (_p.pendingEmitCount <= 0 && _p.pendingSparkEmitCount <= 0) continue;

				// パーティクルマネージャー取得
				auto* _pParticleManager = a_ctx.pServices->pMainEngine->RefParticleManager();
				if (!_pParticleManager) continue;

				// ---------------------------------------------------------
				// 発生源(位置・方向)を emitSpace に応じて決定
				// ---------------------------------------------------------
				Math::Matrix  _world(_transComp.worldMat);
				Math::Vector3 _pos;
				Math::Vector3 _dir;

				switch (_p.emitSpace)
				{
				case EEmitSpace::WorldMatrix:
					// 付いているオブジェクトのワールド位置と前方向(+Z)
					_pos = _world.Translation();
					_dir = Math::Vector3(_world._31, _world._32, _world._33);
					break;

				case EEmitSpace::LocalOffset:
					// worldMat を基準に、ローカルのオフセット位置・方向を合成
					_pos = Math::Vector3::Transform(Math::Vector3(_p.posOffset), _world);
					_dir = Math::Vector3::TransformNormal(Math::Vector3(_p.emitDir), _world);
					break;

				case EEmitSpace::ReverseVelocity:
				{
					// 進行方向の逆へ吹く(噴射・排気)。
					// 弾やミサイルは見た目の姿勢が進行方向と一致していないので、
					// 行列の軸ではなく実際の速度から向きを取る。
					// VelocityComponent はこのクエリに含めない
					// (持たないエンティティのパーティクルまで止まってしまうため)
					_pos = Math::Vector3::Transform(Math::Vector3(_p.posOffset), _world);

					Engine::ECS::Entity _self = a_pChunk->entityData[_i];
					if (a_ctx.pWorld->HasComponent<VelocityComponent>(_self))
					{
						if (const auto* _pVel = a_ctx.pWorld->RefData<VelocityComponent>(_self))
						{
							_dir = -Math::Vector3(_pVel->value);
						}
					}

					// 止まっている(または速度を持たない)ときは後ろ向き＝ローカル +Z の逆
					if (_dir.LengthSquared() <= 1e-8f)
					{
						_dir = -Math::Vector3(_world._31, _world._32, _world._33);
					}
					break;
				}

				case EEmitSpace::FixedWorld:
				default:
					// 行列を使わず、コンポーネントの絶対座標・方向をそのまま
					_pos = Math::Vector3(_p.worldPos);
					_dir = Math::Vector3(_p.emitDir);
					break;
				}

				// 方向の正規化(スケール成分や 0 ベクトルへの安全策)
				if (_dir.LengthSquared() > 1e-8f)
				{
					_dir.Normalize();
				}
				else
				{
					_dir = Math::Vector3(0.0f, 0.0f, 1.0f);
				}

				// ---------------------------------------------------------
				// エミットデータ構築
				// 本体と火花で発生源は共通、形状とアセットだけが違う
				// ---------------------------------------------------------
				auto _requestEmit = [&]
				(
					const Engine::Handle<Engine::Resource::ParticlesAsset>& a_handle,
					int   a_count,
					float a_baseScale,
					float a_minScale,
					float a_maxScale,
					float a_positionRadius,
					float a_directionAngle
					)
				{
					if (a_count <= 0) return;

					// パーティクルアセット取得
					auto* _pParticle = a_ctx.pServices->pResourceManager->Get(a_handle);
					if (!_pParticle) return;

					Engine::Particle::EmitterData _emitData = {};

					_emitData.emitPos       = _pos;
					_emitData.emitDirection = _dir;
					_emitData.emitCount     = static_cast<UINT>(a_count);

					// 形状(スケール/拡散)はコンポーネントから
					_emitData.baseScale      = a_baseScale;
					_emitData.positionRadius = a_positionRadius;
					_emitData.directionAngle = DirectX::XMConvertToRadians(a_directionAngle);
					_emitData.minScale       = a_minScale;
					_emitData.maxScale       = a_maxScale;

					// 速度・寿命はアセットから
					_emitData.minSpeed    = _pParticle->GetInitalSpeedMin();
					_emitData.maxSpeed    = _pParticle->GetInitalSpeedMax();
					_emitData.minLifeTime = _pParticle->GetLifeTimeMin();
					_emitData.maxLifeTime = _pParticle->GetLifeTimeMax();

					// 登録
					_pParticleManager->RequestEmit(a_handle, _emitData);
				};

				// 本体(噴射)
				_requestEmit(
					_p.particlesAssetHandle,
					_p.pendingEmitCount,
					_p.baseScale, _p.minScale, _p.maxScale,
					_p.positionRadius, _p.directionAngle);

				// 火花(点火/消火)
				_requestEmit(
					_p.sparkAssetHandle,
					_p.pendingSparkEmitCount,
					_p.sparkBaseScale, _p.sparkMinScale, _p.sparkMaxScale,
					_p.sparkPositionRadius, _p.sparkDirectionAngle);
			}
		}
	);
};
