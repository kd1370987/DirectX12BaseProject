#include "GunShootSystem.h"

#include "Engine/ECS/World/World.h"
#include "Engine/Resource/Manager/ResourceManager/ResourceManager.h"
#include "Engine/Resource/Data/Prefab/Prefab.h"

#include "../../../../Components/Intent/ActionIntentComponent.h"
#include "../../../../Components/Charactor/Weapon/Gun/GunStateComponent.h"
#include "../../../../Components/Transform/WorldMatrixComponent.h"
#include "../../../../Components/Transform/LocalTransformComponent.h"
#include "../../../../Components/Force/VelocityComponent.h"
#include "../../../../Components/Charactor/AimTargetPosComponent.h"

//==========================================================================================
// GunShootSystem
//
// GunStateComponent を持つエンティティが、発射入力(ActionIntentComponent::isGunShoot)に
// 応じて、設定されたプレハブを「弾」として生成する。
// 生成はシステム反復中に即時に行えない(アーキタイプが壊れる)ため、
// World の遅延生成コマンド(AddEntityWithData)に積み、BegineFrame で安全に生成する。
//==========================================================================================
void GunShootSystem::Init(Engine::ECS::World& a_world)
{
	a_world.ActiveTask<GunStateComponent, const ActionIntentComponent, const WorldMatrixComponent>(
		Engine::ECS::ESystemType::Update,
		"GunShootSystem",
		[]
		(
			Engine::ECS::ArchetypeChunk* a_pChunk,
			uint32_t a_count,
			const Engine::ECS::SystemContext& a_ctx,
			ActiveTag* a_tags,
			GunStateComponent* a_gunArray,
			const ActionIntentComponent* a_intentArray,
			const WorldMatrixComponent* a_worldMatArray
			)
		{
			for (size_t _i = 0; _i < a_count; ++_i)
			{
				GunStateComponent& _gun = a_gunArray[_i];
				const ActionIntentComponent& _intent = a_intentArray[_i];
				const WorldMatrixComponent& _worldMat = a_worldMatArray[_i];

				// 発射判定 : 単発はエッジ、オートは押しっぱなしで毎フレーム
				bool _fire = _intent.isGunShoot && (_gun.isAuto || !_gun.prevShoot);
				_gun.prevShoot = _intent.isGunShoot;
				if (!_fire) continue;

				// プレハブ未設定ならスキップ
				if (_gun.bulletPrefabGUID == Engine::DefaultGUID) continue;

				// プレハブのハンドルを解決(未ロードならロード)
				auto& _rm = *a_ctx.pServices->pResourceManager;
				if (!_rm.IsValid(_gun.bulletPrefabHandle))
				{
					if (!_rm.Has<Engine::Resource::Prefab>(_gun.bulletPrefabGUID))
					{
						_rm.Load<Engine::Resource::Prefab>(_gun.bulletPrefabGUID);
					}
					_gun.bulletPrefabHandle = _rm.GetCache<Engine::Resource::Prefab>(_gun.bulletPrefabGUID);
				}
				auto* _pPrefab = _rm.Ref(_gun.bulletPrefabHandle);
				if (!_pPrefab) continue;

				// ---- 銃の位置と、銃自身のローカル +Z 軸 ----
				const DirectX::XMFLOAT4X4& _m = _worldMat.worldMat;
				DXSM::Vector3 _pos = { _m._41, _m._42, _m._43 };	// 平行移動
				DXSM::Vector3 _gunFwd = { _m._31, _m._32, _m._33 };	// ローカル +Z 軸

				float _gunFwdLenSq = _gunFwd.LengthSquared();
				if (_gunFwdLenSq > 1e-8f) _gunFwd /= std::sqrt(_gunFwdLenSq);

				//======================================================================
				// 基準の向きを決める
				//----------------------------------------------------------------------
				// 銃はアニメーションノードに追従しているため、ローカル +Z 軸が実際の
				// 銃口方向とは限らない(ボーンの軸がそのまま出る)。
				// そのため狙点がある時は、銃の軸ではなく「狙いの向き(カメラ前方)」を
				// 銃口オフセットと後方判定の基準にする。
				//
				// AimTargetPosComponent は AimTargetSystem が計算し、
				// AttachmentDispatchSystem が親から配信してくる。
				// 付いていない銃は今まで通り自分の +Z 軸へ撃つ。
				//======================================================================
				const AimTargetPosComponent* _pAim = nullptr;
				Engine::ECS::Entity _self = a_pChunk->entityData[_i];
				if (a_ctx.pWorld->HasComponent<AimTargetPosComponent>(_self))
				{
					_pAim = a_ctx.pWorld->RefData<AimTargetPosComponent>(_self);

					// まだ一度も計算されていない(=原点が入っている)なら使わない
					if (_pAim && !_pAim->isValid) _pAim = nullptr;
				}

				DXSM::Vector3 _baseDir = _gunFwd;
				if (_pAim)
				{
					DXSM::Vector3 _aimDir = DXSM::Vector3(_pAim->dir);
					float _aimDirLenSq = _aimDir.LengthSquared();
					if (_aimDirLenSq > 1e-8f) _baseDir = _aimDir / std::sqrt(_aimDirLenSq);
				}

				// 銃口を基準の向きへ少し前に出した位置から発射
				DXSM::Vector3 _spawnPos = _pos + _baseDir * 1.5f;

				//======================================================================
				// 射出方向 : 銃口から狙点へ向ける
				//======================================================================
				DXSM::Vector3 _shootDir = _baseDir;
				if (_pAim)
				{
					DXSM::Vector3 _toTarget = DXSM::Vector3(_pAim->pos) - _spawnPos;

					// 狙点が銃口とほぼ同じ位置だと向きが定まらないので、その時は基準のまま
					if (_toTarget.LengthSquared() > 1e-6f)
					{
						_toTarget.Normalize();

						// 狙点が真後ろにある場合は採用しない。
						// (自機の手前の物を拾ってしまった時に、弾がカメラへ向かって
						//  飛んでいくのを防ぐための保険。基準は必ず狙いの向き)
						if (_toTarget.Dot(_baseDir) > 0.0f)
						{
							_shootDir = _toTarget;
						}
					}
				}

				DirectX::XMFLOAT3 _velValue = _shootDir * _gun.speed;

				// ---- プレハブのデータをコピーして、位置と速度を上書き ----
				Engine::ECS::Signature _sig = _pPrefab->GetSignature();
				auto _data = _pPrefab->GetDataMap();	// コピー(型ID -> バイト列)

				auto _ltID = a_ctx.pWorld->GetCompTypeID<LocalTransformComponent>();
				auto _velID = a_ctx.pWorld->GetCompTypeID<VelocityComponent>();
				auto _wmID = a_ctx.pWorld->GetCompTypeID<WorldMatrixComponent>();

				// 弾が動く・描画されるために最低限必要なコンポーネントが無ければ足す
				auto _ensure = [&](Engine::ECS::ComponentTypeID _id)
				{
					if (_sig.test(_id)) return;
					_sig.set(_id);
					auto& _buf = _data[_id];
					_buf.assign(a_ctx.pWorld->GetComponentMetaData(_id).compAlignSize, 0);
					auto _ctor = a_ctx.pWorld->GetCompFunc(_id).construct;
					if (_ctor) _ctor(_buf.data());
				};
				_ensure(_ltID);
				_ensure(_velID);
				_ensure(_wmID);

				// 位置の上書き
				{
					auto& _buf = _data[_ltID];
					LocalTransformComponent _lt = {};
					std::memcpy(&_lt, _buf.data(), sizeof(_lt));
					_lt.pos = _spawnPos;
					_lt.isDirty = true;
					std::memcpy(_buf.data(), &_lt, sizeof(_lt));
				}
				// 速度の上書き
				{
					auto& _buf = _data[_velID];
					VelocityComponent _v = {};
					std::memcpy(&_v, _buf.data(), sizeof(_v));
					_v.value = _velValue;
					std::memcpy(_buf.data(), &_v, sizeof(_v));
				}

				// 反復中なので即時生成せず、遅延生成コマンドに積む
				a_ctx.pWorld->AddEntityWithData(_sig, std::move(_data));
			}
		}
	);
}
