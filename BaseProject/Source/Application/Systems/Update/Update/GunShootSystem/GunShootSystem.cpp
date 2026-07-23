#include "GunShootSystem.h"

#include "Engine/ECS/World/World.h"
#include "Engine/Resource/Manager/ResourceManager/ResourceManager.h"
#include "Engine/Resource/Data/Prefab/Prefab.h"

#include "../../../../Components/Intent/ActionIntentComponent.h"
#include "../../../../Components/Charactor/Weapon/Gun/GunStateComponent.h"
#include "../../../../Components/Transform/WorldMatrixComponent.h"
#include "../../../../Components/Transform/LocalTransformComponent.h"
#include "../../../../Components/Force/VelocityComponent.h"

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

				// ---- 発射位置・方向をワールド行列から求める ----
				const DirectX::XMFLOAT4X4& _m = _worldMat.worldMat;
				DirectX::XMFLOAT3 _pos = { _m._41, _m._42, _m._43 };	// 平行移動
				DirectX::XMFLOAT3 _fwd = { _m._31, _m._32, _m._33 };	// ローカル +Z 軸(前方)

				float _len = std::sqrt(_fwd.x * _fwd.x + _fwd.y * _fwd.y + _fwd.z * _fwd.z);
				if (_len > 0.0001f) { _fwd.x /= _len; _fwd.y /= _len; _fwd.z /= _len; }

				// 銃口を少し前に出した位置から発射
				DirectX::XMFLOAT3 _spawnPos = {
					_pos.x + _fwd.x * 1.5f,
					_pos.y + _fwd.y * 1.5f,
					_pos.z + _fwd.z * 1.5f,
				};
				DirectX::XMFLOAT3 _velValue = {
					_fwd.x * _gun.speed,
					_fwd.y * _gun.speed,
					_fwd.z * _gun.speed,
				};

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
