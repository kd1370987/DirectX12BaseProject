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

				auto* _pEffect = _pResourceManager->Ref(_comp.effectHandle);
				if (!_pEffect) continue;

				const Math::Matrix _ownerWorld(a_worldMatArray[_i].worldMat);
				const Engine::ECS::Entity _self = a_pChunk->entityData[_i];

				//----------------------------------------------------------
				// 出す側からの上書きを、行列1つにまとめておく
				//
				// アセットは共有なので、取り付け位置や大きさの個体差は
				// コンポーネント側(EffectAssetComponent)から受け取る。
				//   v * Scale * Translate * ownerWorld
				// の順で掛けると「オーナーのローカル空間で、指定位置を中心に拡縮」になる。
				// パーティクルの発生位置もメッシュパーツもこの1つで済む
				//----------------------------------------------------------
				const float _effectScale = (_comp.effectScale > 0.0f) ? _comp.effectScale : 1.0f;

				// 束の長さの倍率。粒の初速へ掛けるので、寿命が同じなら
				// 飛ぶ距離＝噴射の長さがそのまま倍率ぶん伸びる。
				// 太さ(_effectScale)とは別物なので、掛ける先も分けてある
				const float _lengthScale = (_comp.effectLengthScale > 0.0f) ? _comp.effectLengthScale : 1.0f;

				Math::Matrix _effectWorld = _ownerWorld;
				if (_comp.isOverrideTransform || _effectScale != 1.0f)
				{
					_effectWorld =
						Math::Matrix::CreateScale(_effectScale) *
						Math::Matrix::CreateTranslation(_comp.overridePosOffset) *
						_ownerWorld;
				}

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
						if (!_pParticle) continue;

						//--------------------------------------------------
						// 発生源(位置・方向)を決める
						//
						// ローカル空間で回すパーティクルは、発生源にくっついて動いてほしいので
						// ワールドではなくその座標系のまま出す。ワールドへ戻すのは描画時。
						// 戻すのに使う行列の席をここで確保しておく。
						//
						// このときパーツの space(WorldMatrix / ReverseVelocity)は使わない。
						// どれも「ワールドのどこに出すか」を決めるものなので、
						// ローカルで回す粒には意味を成さない。
						//--------------------------------------------------
						Math::Vector3 _pos;
						Math::Vector3 _dir;

						UINT _emitterIndex = 0;
						if (_pParticle->IsLocalSpace())
						{
							_emitterIndex = _pParticleManager->AcquireEmitterSlot(
								_part.particleHandle,
								static_cast<uint64_t>(_self),
								_ownerWorld);
						}

						if (_emitterIndex != 0)
						{
							//----------------------------------------------
							// ローカル空間 : 発生源の行列を掛けずに出す
							//----------------------------------------------
							// 取り付け位置とパーツのオフセットだけを合成する。
							// _effectWorld から発生源の行列を除いたものと同じ組み立て
							const Math::Matrix _localMat =
								Math::Matrix::CreateScale(_effectScale) *
								Math::Matrix::CreateTranslation(_comp.overridePosOffset);

							_pos = Math::Vector3::Transform(Math::Vector3(_part.posOffset), _localMat);
							_dir = _comp.isOverrideTransform
								? Math::Vector3(_comp.overrideEmitDir)
								: Math::Vector3(_part.emitDir);
						}
						else
						{
							//----------------------------------------------
							// ワールド空間 : 出した場所にそのまま残る
							//----------------------------------------------
							switch (_part.space)
							{
							case Engine::Resource::EEffectSpace::WorldMatrix:
								// 相手のワールド位置と前方向(+Z)
								_pos = _effectWorld.Translation();
								_dir = Math::Vector3(_ownerWorld._31, _ownerWorld._32, _ownerWorld._33);
								break;

							case Engine::Resource::EEffectSpace::ReverseVelocity:
							{
								// 進行方向の逆へ吹く(噴射・排気)。
								// 弾やミサイルは見た目の姿勢が進行方向と一致しないので、
								// 行列の軸ではなく実際の速度から向きを取る。
								// VelocityComponent はこのクエリに含めない
								// (持たないエンティティのエフェクトまで止まってしまうため)
								_pos = Math::Vector3::Transform(Math::Vector3(_part.posOffset), _effectWorld);

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
								_pos = Math::Vector3::Transform(Math::Vector3(_part.posOffset), _effectWorld);
								_dir = Math::Vector3::TransformNormal(Math::Vector3(_part.emitDir), _ownerWorld);
								break;
							}

							// 向きの上書き : パーツが持っている向きを、出す側の指定で置き換える。
							// 取り付け角度が個体ごとに違うもの(ブースターなど)向け。
							// 位置と違って足し合わせても意味を成さないので、こちらは差し替える
							if (_comp.isOverrideTransform)
							{
								_dir = Math::Vector3::TransformNormal(
									Math::Vector3(_comp.overrideEmitDir), _ownerWorld);
							}
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

						// 大きさとばらつき半径も一緒に拡縮する。
						// 粒だけ大きくして散らばりが元のままだと、束が太らずに粒が重なるだけになる
						_emitData.baseScale      = _part.baseScale * _effectScale;
						_emitData.positionRadius = _part.positionRadius * _effectScale;
						_emitData.directionAngle = DirectX::XMConvertToRadians(_part.directionAngle);
						_emitData.emitShape      = static_cast<UINT>(_part.emitShape);
						_emitData.emitterIndex   = _emitterIndex;
						_emitData.minScale       = _part.minScale;
						_emitData.maxScale       = _part.maxScale;

						// 初速だけ長さの倍率で伸ばす。寿命は触らないので、
						// 束は同じ濃さのまま前へ伸びる(寿命側を伸ばすと尾を引いて残る)
						_emitData.minSpeed    = _pParticle->GetInitalSpeedMin() * _lengthScale;
						_emitData.maxSpeed    = _pParticle->GetInitalSpeedMax() * _lengthScale;
						_emitData.minLifeTime = _pParticle->GetLifeTimeMin();
						_emitData.maxLifeTime = _pParticle->GetLifeTimeMax();

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
							_m, _comp.instance, _effectWorld,
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
