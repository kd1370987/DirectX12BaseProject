#include "EffectDrawSystem.h"

#include "Application/ECS/World/APPWorld.h"
#include "Engine/MainEngine.h"
#include "Engine/Graphics/GraphicsEngine.h"
#include "Engine/Graphics/Particle/ParticleBufferManager.h"
#include "Engine/Graphics/Particle/GPU/EmitterSlotPool/EmitterSlotPool.h"

#include "Application/Components/Effect/EffectRuntimeComponent.h"
#include "Application/Components/Effect/EffectOverrideComponent.h"
#include "Application/Components/Transform/WorldMatrixComponent.h"
#include "Application/Components/Movement/DesiredVelocityComponent.h"

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
void EffectDrawSystem::Init(App::ECS::APPWorld& a_world)
{
	// 発生源の席を取って持つので、EffectRuntimeComponent は書き込み
	a_world.ActiveTask<EffectRuntimeComponent, const EffectOverrideComponent, const WorldMatrixComponent>(
		Engine::ECS::ESystemType::Draw,
		"EffectDrawSystem",
		[]
		(
			Engine::ECS::Chunk* a_pChunk,
			uint32_t a_count,
			const Engine::ECS::SystemContext& a_ctx,
			ActiveTag* a_tags,
			EffectRuntimeComponent* a_runtimeArray,
			const EffectOverrideComponent* a_overrideArray,
			const WorldMatrixComponent* a_worldMatArray
			)
		{
			auto* _pResourceManager = a_ctx.pServices->pResourceManager;
			auto* _pMainEngine = a_ctx.pServices->pMainEngine;
			if (!_pResourceManager || !_pMainEngine) return;

			auto* _pGE = _pMainEngine->RefGraphicsEngine();
			auto* _pParticleManager = _pGE->RefParticleManager();
			auto* _pSlotPool = _pParticleManager ? _pParticleManager->RefEmitterSlotPool() : nullptr;

			for (size_t _i = 0; _i < a_count; ++_i)
			{
				EffectRuntimeComponent& _runtime = a_runtimeArray[_i];
				const EffectOverrideComponent& _override = a_overrideArray[_i];

				auto* _pEffect = _pResourceManager->Ref(_runtime.effectHandle);
				if (!_pEffect) continue;

				const Math::Matrix _ownerWorld(a_worldMatArray[_i].worldMat);
				const Engine::ECS::Entity _self = a_pChunk->entityData[_i];

				//----------------------------------------------------------
				// 発生源の席(ローカル空間のパーティクル用)
				//
				// エフェクト1つにつき1席。Local のパーツが1つでもあれば取る。
				// 行列は再生中かどうか・今フレーム出すかどうかに関係なく毎フレーム書く。
				// 出したフレームだけ書くと、
				//   ・発生レートが fps より低いと、出さないフレームに行列が古いまま描かれてガタつく
				//   ・止めた後の残り粒が、最後に出した位置に置き去りになる
				// 返すのはエンティティが消えるとき(EffectRuntimeComponent の Release)
				//----------------------------------------------------------
				if (_pSlotPool)
				{
					bool _hasLocalPart = false;
					for (const auto& _part : _pEffect->GetParticleParts())
					{
						const auto* _pParticle = _pResourceManager->Get(_part.particleHandle);
						if (_pParticle && _pParticle->IsLocalSpace())
						{
							_hasLocalPart = true;
							break;
						}
					}

					if (_hasLocalPart && !_pSlotPool->IsValid(_runtime.emitterSlot))
					{
						_runtime.emitterSlot = _pSlotPool->Acquire();
					}

					_pSlotPool->SetTransform(_runtime.emitterSlot, _ownerWorld);
				}

				if (!_runtime.instance.isPlaying) continue;

				//----------------------------------------------------------
				// 出す側からの上書きを、行列1つにまとめておく
				//
				// アセットは共有なので、取り付け位置や大きさの個体差は
				// コンポーネント側(EffectAssetComponent)から受け取る。
				//   v * Scale * Translate * ownerWorld
				// の順で掛けると「オーナーのローカル空間で、指定位置を中心に拡縮」になる。
				// パーティクルの発生位置もメッシュパーツもこの1つで済む
				//----------------------------------------------------------
				const float _effectScale = (_override.effectScale > 0.0f) ? _override.effectScale : 1.0f;

				// 束の長さの倍率。粒の初速へ掛けるので、寿命が同じなら
				// 飛ぶ距離＝噴射の長さがそのまま倍率ぶん伸びる。
				// 太さ(_effectScale)とは別物なので、掛ける先も分けてある
				const float _lengthScale = (_override.effectLengthScale > 0.0f) ? _override.effectLengthScale : 1.0f;

				Math::Matrix _effectWorld = _ownerWorld;
				if (_override.isOverrideTransform || _effectScale != 1.0f)
				{
					_effectWorld =
						Math::Matrix::CreateScale(_effectScale) *
						Math::Matrix::CreateTranslation(_override.overridePosOffset) *
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

						const int _emitCount = _runtime.instance.pendingEmit[_p];
						if (_emitCount <= 0) continue;

						// パーティクルアセットが引けなければ出しようがない
						auto* _pParticle = _pResourceManager->Get(_part.particleHandle);
						if (!_pParticle) continue;

						//--------------------------------------------------
						// 発生源(位置・方向)を決める
						//
						// ローカル空間で回すパーティクルは、発生源にくっついて動いてほしいので
						// ワールドではなくその座標系のまま出す。ワールドへ戻すのは描画時。
						// 戻すのに使う行列の席は上で確保・更新してある。
						//
						// このときパーツの space(WorldMatrix / ReverseVelocity)は使わない。
						// どれも「ワールドのどこに出すか」を決めるものなので、
						// ローカルで回す粒には意味を成さない。
						//--------------------------------------------------
						Math::Vector3 _pos;
						Math::Vector3 _dir;

						// 席が取れていなければ 0(単位行列)になり、ワールド空間として出る
						UINT _emitterIndex = 0;
						if (_pParticle->IsLocalSpace())
						{
							_emitterIndex = Engine::Particle::EmitterSlotPool::ToGPUIndex(_runtime.emitterSlot);
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
								Math::Matrix::CreateTranslation(_override.overridePosOffset);

							_pos = Math::Vector3::Transform(Math::Vector3(_part.posOffset), _localMat);
							_dir = _override.isOverrideTransform
								? Math::Vector3(_override.overrideEmitDir)
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
								// DesiredVelocityComponent はこのクエリに含めない
								// (持たないエンティティのエフェクトまで止まってしまうため)
								_pos = Math::Vector3::Transform(Math::Vector3(_part.posOffset), _effectWorld);

								// RefData は持っていないコンポーネントなら nullptr を返す
								if (a_ctx.pWorld->HasComponent<DesiredVelocityComponent>(_self))
								{
									if (const auto* _pVel = a_ctx.pWorld->RefData<DesiredVelocityComponent>(_self))
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
							if (_override.isOverrideTransform)
							{
								_dir = Math::Vector3::TransformNormal(
									Math::Vector3(_override.overrideEmitDir), _ownerWorld);
							}
						}

						//--------------------------------------------------
						// 発生行列にまとめる
						//
						// 形状はシェーダーがローカル(+Z が噴き出す向き)で作って、この行列を掛ける。
						// 噴き出す向きを軸にした回転(ロール)は上の手がかりで決める。
						//   ローカル空間 : 席の座標系の +Y(席自体が持ち主の回転を持っている)
						//   ワールド空間 : 持ち主の +Y(機体が傾けば一緒に傾く)
						// いまの形状(円錐・球・半球と球状のばらつき)は噴き出す向きのまわりで対称なので、
						// ロールの決め方で見た目は変わらない。粒に向きを持たせる 3-C から効いてくる。
						// 方向の正規化と 0 ベクトルの安全策も MakeEmitMatrix が持つ
						//--------------------------------------------------
						const Math::Vector3 _upHint = (_emitterIndex != 0)
							? Math::Vector3(0.0f, 1.0f, 0.0f)
							: Math::Vector3::TransformNormal(Math::Vector3(0.0f, 1.0f, 0.0f), _ownerWorld);

						//--------------------------------------------------
						// エミットデータ構築
						// 散らばり方はエフェクト側、速度と寿命はパーティクルアセット側
						//--------------------------------------------------
						Engine::Particle::EmitterData _emitData = {};

						_emitData.emitMatrix    = Engine::Particle::MakeEmitMatrix(_pos, _dir, _upHint);
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
							_m, _runtime.instance, _effectWorld,
							_meshWorld, _colorScale, _emissiveAdd))
						{
							continue;
						}

						_pGE->RefDrawSubmitter()->SubmitModel(
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
