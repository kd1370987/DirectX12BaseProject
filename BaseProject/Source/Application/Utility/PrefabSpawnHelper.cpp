#include "PrefabSpawnHelper.h"

#include <cstring>

#include "Engine/ECS/World/World.h"
#include "Engine/Resource/Manager/ResourceManager/ResourceManager.h"
#include "Engine/Resource/Data/Prefab/Prefab.h"

#include "../Components/Transform/LocalTransformComponent.h"
#include "../Components/Hierarchy/SpawnerComponent.h"

namespace App::Utility
{
	namespace
	{
		//----------------------------------------------------------------------
		// データマップに指定コンポーネントの領域を用意する。
		// プレハブが持っていなければシグネチャへ足し、既定構築しておく。
		// 返すのはその書き込み先(確保できなければ nullptr)。
		//----------------------------------------------------------------------
		uint8_t* EnsureComponentBuffer(
			Engine::ECS::World& a_world,
			Engine::ECS::Signature& a_sig,
			std::unordered_map<Engine::ECS::ComponentTypeID, std::vector<uint8_t>>& a_data,
			Engine::ECS::ComponentTypeID a_typeID)
		{
			if (a_typeID == Engine::ECS::Limits::INVALID_COMPONENTTYPEID) return nullptr;

			if (!a_sig.test(a_typeID))
			{
				a_sig.set(a_typeID);
				auto& _newBuf = a_data[a_typeID];
				_newBuf.assign(a_world.GetComponentMetaData(a_typeID).compAlignSize, 0);
				auto _ctor = a_world.GetCompFunc(a_typeID).construct;
				if (_ctor) _ctor(_newBuf.data());
			}

			auto& _buf = a_data[a_typeID];
			return _buf.empty() ? nullptr : _buf.data();
		}
	}

	bool SpawnPrefab(
		Engine::ECS::World& a_world,
		Engine::Resource::ResourceManager& a_resourceManager,
		const Engine::GUID& a_prefabGUID,
		Engine::Handle<Engine::Resource::Prefab>& a_refHandle,
		const SpawnParams& a_params)
	{
		if (a_prefabGUID == Engine::DefaultGUID) return false;

		// ハンドルを解決(未ロードならロード)して、参照を1つ取る。
		// 返すのはこのハンドルを持っているコンポーネントの解放フック
		if (!a_resourceManager.IsValid(a_refHandle))
		{
			a_resourceManager.AcquireImmediate(a_refHandle, a_prefabGUID);
		}

		auto* _pPrefab = a_resourceManager.Ref(a_refHandle);
		if (!_pPrefab) return false;

		//------------------------------------------------------------------
		// ルート + 子ぶんの材料を作る
		//------------------------------------------------------------------
		// GUID の振り直しと親子リンクの張り替えはプレハブ側で済んでいる。
		// 先頭がルートで、親が子より前に並んでいる。
		//------------------------------------------------------------------
		std::vector<Engine::Resource::PrefabInstanceData> _instanceVec =
			_pPrefab->BuildInstanceData(&a_world);
		if (_instanceVec.empty()) return false;

		// 子は保存された姿のまま出す(位置と向きは親に追従する)
		for (size_t _i = 1; _i < _instanceVec.size(); ++_i)
		{
			a_world.AddEntityWithData(_instanceVec[_i].sig, std::move(_instanceVec[_i].dataMap));
		}

		Engine::ECS::Signature _sig = _instanceVec[0].sig;
		auto _data = std::move(_instanceVec[0].dataMap);

		// 位置と向きを入れる。LocalTransform が無ければ足す
		if (uint8_t* _pBuf = EnsureComponentBuffer(
			a_world, _sig, _data, a_world.GetCompTypeID<LocalTransformComponent>()))
		{
			LocalTransformComponent _lt = {};
			std::memcpy(&_lt, _pBuf, sizeof(_lt));
			_lt.pos     = a_params.pos;
			if (a_params.isOverrideRotation) _lt.quat = a_params.quat;
			_lt.isDirty = true;
			std::memcpy(_pBuf, &_lt, sizeof(_lt));
		}

		// 生成元の印。出した側が「自分が出した生存エンティティ」を数えるのに使う
		if (a_params.spawnerGUID.IsValid())
		{
			if (uint8_t* _pBuf = EnsureComponentBuffer(
				a_world, _sig, _data, a_world.GetCompTypeID<SpawnerComponent>()))
			{
				SpawnerComponent _spawner = {};
				std::memcpy(&_spawner, _pBuf, sizeof(_spawner));
				_spawner.spawnerGUID = a_params.spawnerGUID;
				_spawner.waveIndex   = a_params.waveIndex;
				std::memcpy(_pBuf, &_spawner, sizeof(_spawner));
			}
		}

		// 反復中なので即時生成せず、遅延生成コマンドに積む
		a_world.AddEntityWithData(_sig, std::move(_data));
		return true;
	}

	bool SpawnPrefabAt(
		Engine::ECS::World& a_world,
		Engine::Resource::ResourceManager& a_resourceManager,
		const Engine::GUID& a_prefabGUID,
		Engine::Handle<Engine::Resource::Prefab>& a_refHandle,
		const Math::Vector3& a_pos)
	{
		// 向きも印も付けない(プレハブの保存値のまま出す)
		SpawnParams _params = {};
		_params.pos = a_pos;

		return SpawnPrefab(a_world, a_resourceManager, a_prefabGUID, a_refHandle, _params);
	}
}
