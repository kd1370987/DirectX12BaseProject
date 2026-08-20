#include "EffectDrawSystem.h"

#include "Engine/ECS/World/World.h"
#include "Engine/MainEngine.h"
#include "Engine/Graphics/GraphicEngine.h"
#include "Engine/Particle/ParticleBufferManager.h"

#include "../../../../Components/Effect/EffectAssetComponent.h"
#include "../../../../Components/Transform/WorldMatrixComponent.h"
#include "../../../../Components/Force/VelocityComponent.h"

//==========================================================================================
// EffectDrawSystem
//
// EffectUpdateSystem が決めたぶんを、実際に出す。
//
//   パーティクル : パーツごとの pendingEmit 個を GPU へ emit 要求する
//   メッシュ     : パーツごとの行列と色を組んで描画命令に積む
//
// どちらも発生源はエフェクトが付いているエンティティのワールド行列。
// パーツ側は「そこからどうずらすか」しか持たないので、
// 同じエフェクトを別の場所・別の相手に付け回せる。
//==========================================================================================
void EffectDrawSystem::Init(Engine::ECS::World& a_world)
{
	a_world.ActiveTask<const EffectAssetComponent, const WorldMatrixComponent>(
		Engine::ECS::ESystemType::Draw,
		"EffectDrawSystem",
		[]
		(
			Engine::ECS::ArchetypeChunk* a_pChunk,
			uint32_t a_count,
			const Engine::ECS::SystemContext& a_ctx,
			ActiveTag* a_tags,
			const EffectAssetComponent* a_effectArray,
			const WorldMatrixComponent* a_worldMatArray
			)
		{
			auto* _pResourceManager = a_ctx.pServices->pResourceManager;
			auto* _pMainEngine = a_ctx.pServices->pMainEngine;
			if (!_pResourceManager || !_pMainEngine) return;

			auto* _pGE = _pMainEngine->RefGraphicsEngine();
			auto* _pParticleManager = _pMainEngine->RefParticleManager();

			for (size_t _i = 0; _i < a_count; ++_i)
			{
				const EffectAssetComponent& _comp = a_effectArray[_i];
				if (!_comp.instance.isPlaying) continue;

				if (!_pParticleManager) ENGINE_LOG("[DBG-Draw] ParticleManager が null");

				auto* _pEffect = _pResourceManager->Ref(_comp.effectHandle);
				if (!_pEffect) continue;

				const Math::Matrix _ownerWorld(a_worldMatArray[_i].worldMat);
				const Engine::ECS::Entity _self = a_pChunk->entityData[_i];

				//----------------------------------------------------------
				// パーティクル
				//----------------------------------------------------------
				if (_pParticleManager)
				{
					const auto& _particleParts = _pEffect->GetParticleParts();
					const size_t _partCount =
						std::min<size_t>(_particleParts.size(), Engine::Resource::EFFECT_PARTICLE_MAX);

					for (size_t _p = 0; _p < _partCount; ++_p)
					{
						const auto& _part = _particleParts[_p];

						const int _emitCount = _comp.instance.pendingEmit[_p];
						if (_emitCount <= 0) continue;

						// パーティクルアセットが引けなければ出しようがない
						auto* _pParticle = _pResourceManager->Get(_part.particleHandle);
						if (!_pParticle)
						{
							ENGINE_LOG("[DBG-Draw] part%d パーティクルが引けない (handleValid=%d)",
								static_cast<int>(_p), _part.particleHandle.IsValid() ? 1 : 0);
							continue;
						}

						//--------------------------------------------------
						// 発生源(位置・方向)を space に応じて決める
						//--------------------------------------------------
						Math::Vector3 _pos;
						Math::Vector3 _dir;

						switch (_part.space)
						{
						case Engine::Resource::EEffectSpace::WorldMatrix:
							// 相手のワールド位置と前方向(+Z)
							_pos = _ownerWorld.Translation();
							_dir = Math::Vector3(_ownerWorld._31, _ownerWorld._32, _ownerWorld._33);
							break;

						case Engine::Resource::EEffectSpace::ReverseVelocity:
						{
							// 進行方向の逆へ吹く(噴射・排気)。
							// 弾やミサイルは見た目の姿勢が進行方向と一致しないので、
							// 行列の軸ではなく実際の速度から向きを取る。
							// VelocityComponent はこのクエリに含めない
							// (持たないエンティティのエフェクトまで止まってしまうため)
							_pos = Math::Vector3::Transform(Math::Vector3(_part.posOffset), _ownerWorld);

							// RefData は持っていないコンポーネントでも非nullを返すので、
							// 必ず HasComponent で確かめてから引くこと
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
								_dir = -Math::Vector3(_ownerWorld._31, _ownerWorld._32, _ownerWorld._33);
							}
							break;
						}

						case Engine::Resource::EEffectSpace::LocalOffset:
						default:
							// 相手の行列を基準に、ローカルのオフセット位置・方向を合成
							_pos = Math::Vector3::Transform(Math::Vector3(_part.posOffset), _ownerWorld);
							_dir = Math::Vector3::TransformNormal(Math::Vector3(_part.emitDir), _ownerWorld);
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

						//--------------------------------------------------
						// エミットデータ構築
						// 散らばり方はエフェクト側、速度と寿命はパーティクルアセット側
						//--------------------------------------------------
						Engine::Particle::EmitterData _emitData = {};

						_emitData.emitPos       = _pos;
						_emitData.emitDirection = _dir;
						_emitData.emitCount     = static_cast<UINT>(_emitCount);

						_emitData.baseScale      = _part.baseScale;
						_emitData.positionRadius = _part.positionRadius;
						_emitData.directionAngle = DirectX::XMConvertToRadians(_part.directionAngle);
						_emitData.minScale       = _part.minScale;
						_emitData.maxScale       = _part.maxScale;

						_emitData.minSpeed    = _pParticle->GetInitalSpeedMin();
						_emitData.maxSpeed    = _pParticle->GetInitalSpeedMax();
						_emitData.minLifeTime = _pParticle->GetLifeTimeMin();
						_emitData.maxLifeTime = _pParticle->GetLifeTimeMax();

						ENGINE_LOG("[DBG-Draw] part%d RequestEmit n=%d pos=(%.2f,%.2f,%.2f)",
							static_cast<int>(_p), _emitCount, _pos.x, _pos.y, _pos.z);
						_pParticleManager->RequestEmit(_part.particleHandle, _emitData);
					}
				}

				//----------------------------------------------------------
				// メッシュ
				//----------------------------------------------------------
				if (_pGE)
				{
					const auto& _meshParts = _pEffect->GetMeshParts();

					for (size_t _m = 0; _m < _meshParts.size(); ++_m)
					{
						auto* _pModel = _pResourceManager->Get(_meshParts[_m].modelHandle);
						if (!_pModel) continue;

						// 今出している時間帯かどうかも含めて、アセット側が組んでくれる
						Math::Matrix  _meshWorld;
						Math::Color   _colorScale;
						Math::Vector3 _emissiveAdd;
						if (!_pEffect->BuildMeshDraw(
							_m, _comp.instance, _ownerWorld,
							_meshWorld, _colorScale, _emissiveAdd))
						{
							continue;
						}

						_pGE->SubmitModel(
							*a_ctx.pWorld,
							_pModel,
							_meshWorld,
							_colorScale,
							{ 1.0f, 1.0f, 1.0f },	// エミッシブテクスチャの倍率は素通し
							_emissiveAdd
						);
					}
				}
			}
		}
	);
}
