#include "MissileSalvoSystem.h"

#include "Engine/ECS/World/World.h"
#include "Engine/Option/OptionManager.h"
#include "Engine/Resource/Manager/ResourceManager/ResourceManager.h"
#include "Engine/Resource/Data/Prefab/Prefab.h"
#include "Engine/Resource/Data/Model/Model.h"

#include "Application/Components/Tag/ActiveCameraTag.h"
#include "Application/Components/Tag/CameraTag.h"
#include "Application/Components/Tag/EnemyTag.h"
#include "Application/Components/Camera/ProjMatComponent.h"
#include "Application/Components/Intent/ActionIntentComponent.h"
#include "Application/Components/Character/AimTargetPosComponent.h"
#include "Application/Components/Character/Robot/AttachmentSlotsComponent.h"
#include "Application/Components/Character/Weapon/Missile/MissileLockComponent.h"
#include "Application/Components/Character/Weapon/Gun/GunStateComponent.h"
#include "Application/Components/Resource/ModelComponent.h"
#include "Application/Components/Transform/WorldMatrixComponent.h"

#include "../../../Shared/ProjectileSpawn/ProjectileSpawn.h"

//==========================================================================================
// MissileSalvoSystem
//
// ミサイルキー(ActionIntentComponent::isMissileHold)を
//   押している間 … コンバットレティクルの円に入った敵を MissileLockComponent へ溜める
//   離した瞬間   … 溜めた相手へ missileCount 発を割り振り、発射キューへ積む
//   その後       … launchInterval 間隔で1発ずつ実際に生成する
// という流れを1つのシステムで面倒を見る。
//
// ・PostUpdate に置く理由
//     収集はスクリーン座標で行うので、カメラと敵の「今フレームの」ワールド行列が要る。
//     WorldMatrixComponent を読むので CalcMatrixSystem /
//     CommitHierarchyWorldMatrixSystem より自動的に後ろへ回る(LockOnTargetSystem と同じ)。
//     発射位置もミサイルポッドの今フレームの行列から取るため、同じ帯にあるのが都合が良い。
// ・収集と発射を分けなかったのは、両方が MissileLockComponent を書くため。
//   システム順のグラフは RAW(片方が読む)しか辺にしないので、書き手同士は
//   登録順頼みになってしまう。1システムに閉じておけば順序の心配が要らない。
// ・弾のプレハブ・弾速・銃口ノードはミサイルポッド側の GunStateComponent を使う。
//   ポッドは既に銃と同じ部品(GunState / Model / WorldMatrix)を持っているので、
//   ミサイル専用に同じ設定をもう一組作らない。
//==========================================================================================
namespace
{
	// 一斉射の弾を散らす向きを作る
	//   基準方向を軸にしたコーンの側面へ、発射順で回しながら並べる。
	//   散らしたあとは HomingSystem が相手へ寄せるので、
	//   「いったん広がってから食いつく」いつもの見た目になる。
	Math::Vector3 MakeSpreadDir(
		const Math::Vector3& a_baseDir,
		float                a_spreadRad,
		int                  a_index,
		int                  a_total)
	{
		if (a_spreadRad <= 0.0f || a_total <= 0) return a_baseDir;

		// 基準方向に直交する基底を作る。真上を向いている時だけ参照軸を変える
		const Math::Vector3 _ref = (std::fabs(a_baseDir.y) > 0.99f)
			? Math::Vector3(1.0f, 0.0f, 0.0f)
			: Math::Vector3(0.0f, 1.0f, 0.0f);

		Math::Vector3 _right = _ref.Cross(a_baseDir);
		if (_right.LengthSquared() <= 1e-8f) return a_baseDir;
		_right.Normalize();

		Math::Vector3 _up = a_baseDir.Cross(_right);
		_up.Normalize();

		// コーンの周りを等間隔に回す
		const float _azimuth = DirectX::XM_2PI * static_cast<float>(a_index) / static_cast<float>(a_total);

		Math::Vector3 _dir =
			a_baseDir * std::cos(a_spreadRad) +
			(_right * std::cos(_azimuth) + _up * std::sin(_azimuth)) * std::sin(a_spreadRad);

		_dir.Normalize();
		return _dir;
	}
}

void MissileSalvoSystem::Init(Engine::ECS::World& a_world)
{
	a_world.ActiveTask<MissileLockComponent, const AttachmentSlotsComponent,
		const ActionIntentComponent, const WorldMatrixComponent>(
		Engine::ECS::ESystemType::PostUpdate,
		"MissileSalvoSystem",
		[](
			Engine::ECS::ArchetypeChunk* a_pChunk,
			uint32_t a_count,
			const Engine::ECS::SystemContext& a_ctx,
			ActiveTag* a_tags,
			MissileLockComponent* a_missileArray,
			const AttachmentSlotsComponent* a_slotsArray,
			const ActionIntentComponent* a_intentArray,
			const WorldMatrixComponent* a_worldMatArray
		)
		{
			if (a_count == 0) return;
			if (!a_ctx.pServices || !a_ctx.pServices->pOptionManager) return;

			// スクリーン解像度(px) : HUD の SubmitUI がピクセル座標を解釈する基準と合わせる
			const auto& _winOp = a_ctx.pServices->pOptionManager->GetWindowOption();
			const float _w = static_cast<float>(_winOp.windowWidth);
			const float _h = static_cast<float>(_winOp.windowHeight);
			if (_w <= 0.0f || _h <= 0.0f) return;

			//==================================================================
			// アクティブカメラから ViewProj を作る(LockOnTargetSystem と同じ)
			//==================================================================
			Math::Matrix _viewProjMat = Math::Matrix::Identity();
			bool _hasCamera = false;

			a_ctx.pWorld->ForEach<const ActiveCameraTag, const CameraTag, const ProjMatComponent, const WorldMatrixComponent>(
				[&](
					Engine::ECS::ArchetypeChunk* a_pCamChunk,
					uint32_t a_camCount,
					const ActiveCameraTag* a_activeCamTagArray,
					const CameraTag* a_camTagArray,
					const ProjMatComponent* a_projMatArray,
					const WorldMatrixComponent* a_camWorldMatArray
				)
				{
					// アクティブカメラは1台の想定。先に見つかったものを使う
					if (_hasCamera || a_camCount == 0) return;

					const Math::Matrix _camWorldMat = a_camWorldMatArray[0].worldMat;
					const Math::Matrix _projMat     = a_projMatArray[0].projMat;

					_viewProjMat = _camWorldMat.Invert() * _projMat;
					_hasCamera   = true;
				}
			);

			const Math::Vector2 _defaultCenter = { _w * 0.5f, _h * 0.5f };

			for (size_t _i = 0; _i < a_count; ++_i)
			{
				MissileLockComponent&           _missile   = a_missileArray[_i];
				const AttachmentSlotsComponent& _slots     = a_slotsArray[_i];
				const ActionIntentComponent&    _intent    = a_intentArray[_i];
				const WorldMatrixComponent&     _selfWorld = a_worldMatArray[_i];

				const Engine::ECS::Entity _self = a_pChunk->entityData[_i];

				// クールダウンを進める
				if (_missile.cooldownTimer > 0.0f)
				{
					_missile.cooldownTimer =
						std::max(_missile.cooldownTimer - a_ctx.dt, 0.0f);
				}

				const Math::Matrix& _pm = _selfWorld.worldMat;
				const Math::Vector3 _selfPos = { _pm._41, _pm._42, _pm._43 };

				//==============================================================
				// 1. ターゲットの収集
				//--------------------------------------------------------------
				// 押している間だけ溜める。発射中とクールダウン中は次の収集を
				// 始めさせない(撃ち終わる前に溜め直せてしまうため)
				//==============================================================
				// 押し離しの検出は入力そのもので見る。溜められるかどうか(_canCharge)を
				// 混ぜると、クールダウンに入った次のフレームが「離した」扱いになってしまう
				const bool _rawHold = _intent.isMissileHold;
				const bool _canCharge =
					!_missile.IsFiring() && !_missile.IsCoolDown() && _hasCamera;
				const bool _isHold = _rawHold && _canCharge;

				if (_isHold)
				{
					_missile.isCharging = true;

					// 判定の円。CombatReticleHUD が居ればそれに合わせる
					const Math::Vector2 _reticleCenter = _missile.isReticleFromHUD
						? Math::Vector2(_missile.reticleCenter)
						: _defaultCenter;
					const float _reticleRadius = _missile.GetActiveReticleRadius();
					const int   _lockMax       = _missile.GetActiveMissileCount();

					//----------------------------------------------------------
					// 消滅した敵をロックから外す(前へ詰める)
					//----------------------------------------------------------
					{
						int _write = 0;
						for (int _r = 0; _r < _missile.lockCount; ++_r)
						{
							const Engine::ECS::Entity _e = _missile.locks[_r];
							if (_e == Engine::ECS::Limits::INVALID_ENTITY) continue;
							if (!a_ctx.pWorld->HasComponent<WorldMatrixComponent>(_e)) continue;

							_missile.locks[_write]         = _e;
							_missile.lockScreenPos[_write] = _missile.lockScreenPos[_r];
							++_write;
						}
						_missile.lockCount = _write;
					}

					//----------------------------------------------------------
					// 円の内側に居る敵を溜める
					// 一度ロックした相手は円から出ても外さない(溜める操作なので)
					//----------------------------------------------------------
					a_ctx.pWorld->ForEach<const ActiveTag, const EnemyTag, const WorldMatrixComponent>(
						[&](
							Engine::ECS::ArchetypeChunk* a_pEnemyChunk,
							uint32_t a_enemyCount,
							const ActiveTag* a_activeTagArray,
							const EnemyTag* a_enemyTagArray,
							const WorldMatrixComponent* a_enemyWorldMatArray
						)
						{
							for (uint32_t _e = 0; _e < a_enemyCount; ++_e)
							{
								const Math::Matrix& _em = a_enemyWorldMatArray[_e].worldMat;
								Math::Vector3 _worldPos = { _em._41, _em._42, _em._43 };
								_worldPos.y += _missile.targetOffsetY;

								// 距離で足切り(0 以下なら距離では切らない)
								if (_missile.maxDistance > 0.0f)
								{
									if ((_worldPos - _selfPos).Length() > _missile.maxDistance) continue;
								}

								// ワールド → スクリーン
								Math::Vector4 _clipPos = Math::Vector4::Transform(_worldPos, _viewProjMat);

								// w <= 0 はカメラ後方。割ると画面内へ折り返して映るので弾く
								if (_clipPos.w <= 1e-4f) continue;

								const float _invW = 1.0f / _clipPos.w;
								const Math::Vector3 _ndc = {
									_clipPos.x * _invW, _clipPos.y * _invW, _clipPos.z * _invW };

								// ニア/ファーの外は狙わない(DirectXの深度は0..1)
								if (_ndc.z < 0.0f || _ndc.z > 1.0f) continue;

								// NDC(中心原点/Y上向き) → ピクセル(左上原点/Y下向き)
								const Math::Vector2 _screenPos = {
									(_ndc.x * 0.5f + 0.5f) * _w,
									(0.5f - _ndc.y * 0.5f) * _h
								};

								// レティクルの内側か
								if ((_screenPos - _reticleCenter).Length() > _reticleRadius) continue;

								// すでにロック済みなら座標だけ更新する(HUD 用)
								const Engine::ECS::Entity _entity = a_pEnemyChunk->entityData[_e];
								bool _found = false;
								for (int _l = 0; _l < _missile.lockCount; ++_l)
								{
									if (_missile.locks[_l] != _entity) continue;
									_missile.lockScreenPos[_l] = _screenPos;
									_found = true;
									break;
								}
								if (_found) continue;

								// 新規ロック(1体につき1つ。弾数ぶん溜まったら打ち止め)
								if (_missile.lockCount >= _lockMax) continue;

								_missile.locks[_missile.lockCount]         = _entity;
								_missile.lockScreenPos[_missile.lockCount] = _screenPos;
								++_missile.lockCount;
							}
						}
					);
				}
				else
				{
					_missile.isCharging = false;
				}

				//==============================================================
				// 2. 離した瞬間 : 発射キューを作る
				//--------------------------------------------------------------
				// ロックした相手へ順番に割り振る。敵の数が弾数に足りなければ
				// 先頭から使い回すので、1体しか居なければ全弾がその1体へ向かう。
				// 1つもロックできていない時は、既定では誘導なしで撃つ
				// (requireLock を立てると撃たない)。
				//==============================================================
				const bool _released = _missile.wasHold && !_rawHold;
				_missile.wasHold = _rawHold;

				if (_released)
				{
					const int _fireCount = _missile.GetActiveMissileCount();
					const bool _canFire =
						!_missile.IsFiring() && !_missile.IsCoolDown() &&
						(_fireCount > 0) && (!_missile.requireLock || _missile.lockCount > 0);

					if (_canFire)
					{
						for (int _m = 0; _m < _fireCount; ++_m)
						{
							_missile.fireTargets[_m] = (_missile.lockCount > 0)
								? _missile.locks[_m % _missile.lockCount]
								: Engine::ECS::Limits::INVALID_ENTITY;
						}

						_missile.fireTotal  = _fireCount;
						_missile.fireRemain = _fireCount;
						_missile.fireTimer  = 0.0f;

						// クールダウンは撃ち始めから数える
						_missile.cooldownTimer = std::max(_missile.cooldown, 0.0f);
					}

					// 溜めたぶんは撃つ / 撃たないに関わらず捨てる
					_missile.lockCount  = 0;
					_missile.isCharging = false;
				}

				//==============================================================
				// 3. 発射キューの消化
				//==============================================================
				if (!_missile.IsFiring()) continue;

				_missile.fireTimer -= a_ctx.dt;
				if (_missile.fireTimer > 0.0f) continue;

				// ---- ミサイルポッド(発射する武器)を引く ----
				const Engine::ECS::Entity _podEntity = _slots.missile.id;
				if (_podEntity == Engine::ECS::Limits::INVALID_ENTITY)
				{
					// 武器が付いていないなら撃てない。キューは捨てる
					_missile.fireRemain = 0;
					continue;
				}
				if (!a_ctx.pWorld->HasComponent<GunStateComponent>(_podEntity) ||
					!a_ctx.pWorld->HasComponent<WorldMatrixComponent>(_podEntity))
				{
					_missile.fireRemain = 0;
					continue;
				}

				auto* _pGun      = a_ctx.pWorld->RefData<GunStateComponent>(_podEntity);
				auto* _pPodWorld = a_ctx.pWorld->RefData<WorldMatrixComponent>(_podEntity);
				if (!_pGun || !_pPodWorld)
				{
					_missile.fireRemain = 0;
					continue;
				}

				// プレハブ未設定なら撃てない
				if (_pGun->bulletPrefabGUID == Engine::DefaultGUID)
				{
					_missile.fireRemain = 0;
					continue;
				}

				// プレハブのハンドルを解決(未ロードならロード)
				auto& _rm = *a_ctx.pServices->pResourceManager;
				if (!_rm.IsValid(_pGun->bulletPrefabHandle))
				{
					if (!_rm.Has<Engine::Resource::Prefab>(_pGun->bulletPrefabGUID))
					{
						_rm.LoadImmediate<Engine::Resource::Prefab>(_pGun->bulletPrefabGUID);
					}
					_pGun->bulletPrefabHandle = _rm.GetCache<Engine::Resource::Prefab>(_pGun->bulletPrefabGUID);
				}
				auto* _pPrefab = _rm.Ref(_pGun->bulletPrefabHandle);
				if (!_pPrefab) continue;

				//--------------------------------------------------------------
				// 発射位置 : 銃口ヌルノードが設定されていればそこから
				// (node.worldTransform はモデルルート基準なので、ポッドの
				//  ワールド行列で変換してワールド座標にする。GunShootSystem と同じ)
				//--------------------------------------------------------------
				const Math::Matrix& _podMat = _pPodWorld->worldMat;
				Math::Vector3 _spawnPos = { _podMat._41, _podMat._42, _podMat._43 };

				if (_pGun->nullPtrNodeHash != 0 &&
					a_ctx.pWorld->HasComponent<ModelComponent>(_podEntity))
				{
					if (const auto* _pModelComp = a_ctx.pWorld->RefData<ModelComponent>(_podEntity))
					{
						auto* _pModel = a_ctx.pServices->pResourceManager->Get(_pModelComp->handle);
						if (_pModel)
						{
							const auto& _nodeVec = _pModel->GetOriginalNodeVec();
							if (_pGun->nodeIndex < _nodeVec.size())
							{
								const Math::Matrix& _nodeMat = _nodeVec[_pGun->nodeIndex].worldTransform;
								Math::Vector3 _nodeLocalPos = { _nodeMat._41, _nodeMat._42, _nodeMat._43 };
								_spawnPos = Math::Vector3::Transform(_nodeLocalPos, Math::Matrix(_podMat));
							}
						}
					}
				}

				//--------------------------------------------------------------
				// 基準の向き
				//   狙点(カメラのレイ)があればそちら、無ければ自機の前方(+Z)。
				//   ロックしている相手が居る弾は、その相手への向きを基準にする
				//--------------------------------------------------------------
				Math::Vector3 _aimDir = { _pm._31, _pm._32, _pm._33 };	// 左手系 +Z = 前方
				if (a_ctx.pWorld->HasComponent<AimTargetPosComponent>(_self))
				{
					if (const auto* _pAim = a_ctx.pWorld->RefData<AimTargetPosComponent>(_self))
					{
						if (_pAim->isValid)
						{
							Math::Vector3 _d(_pAim->dir);
							if (_d.LengthSquared() > 1e-8f) _aimDir = _d;
						}
					}
				}
				if (_aimDir.LengthSquared() > 1e-8f) _aimDir.Normalize();
				else                                 _aimDir = Math::Vector3(0.0f, 0.0f, 1.0f);

				const float _spreadRad = DirectX::XMConvertToRadians(std::max(_missile.spreadAngle, 0.0f));
				const Engine::ECS::Entity _shooter =
					App::Systems::ProjectileSpawn::ResolveShooterEntity(*a_ctx.pWorld, _podEntity);

				//--------------------------------------------------------------
				// 溜まっているぶんを撃つ
				//   launchInterval が 0 なら timer が進まないので同フレームに全弾出る
				//--------------------------------------------------------------
				while (_missile.fireRemain > 0 && _missile.fireTimer <= 0.0f)
				{
					// 何発目か(散らしの角度を弾ごとにずらすため)
					const int _index = _missile.fireTotal - _missile.fireRemain;

					const Engine::ECS::Entity _target = _missile.fireTargets[_index];

					// 相手が居るならそちらを基準に、居なければ狙いの向きを基準に散らす
					Math::Vector3 _baseDir = _aimDir;
					if (_target != Engine::ECS::Limits::INVALID_ENTITY &&
						a_ctx.pWorld->HasComponent<WorldMatrixComponent>(_target))
					{
						if (const auto* _pTargetWorld = a_ctx.pWorld->RefData<WorldMatrixComponent>(_target))
						{
							Math::Vector3 _targetPos =
								Math::Matrix(_pTargetWorld->worldMat).Translation();
							_targetPos.y += _missile.targetOffsetY;

							Math::Vector3 _toTarget = _targetPos - _spawnPos;
							if (_toTarget.LengthSquared() > 1e-6f)
							{
								_toTarget.Normalize();
								_baseDir = _toTarget;
							}
						}
					}

					const Math::Vector3 _shootDir =
						MakeSpreadDir(_baseDir, _spreadRad, _index, _missile.fireTotal);

					App::Systems::ProjectileSpawn::Spawn(
						*a_ctx.pWorld,
						_pPrefab,
						_spawnPos,
						_shootDir * _pGun->speed,
						_shooter,
						_target);

					--_missile.fireRemain;
					_missile.fireTimer += std::max(_missile.launchInterval, 0.0f);
				}
			}
		}
	);
}
