#include "PrefabSpawnHelper.h"

#include <cstring>

#include "Engine/ECS/World/World.h"
#include "Engine/Resource/Manager/ResourceManager/ResourceManager.h"
#include "Engine/Resource/Data/Prefab/Prefab.h"

#include "../Components/Transform/LocalTransformComponent.h"
#include "../Components/Hierarchy/SpawnerComponent.h"
#include "../Components/Camera/FollowTargetComponent.h"

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

		std::vector<Engine::Resource::PrefabInstanceData> _instanceVec = {};
		if (!BuildSpawnInstanceData(a_world, *_pPrefab, a_params, _instanceVec)) return false;

		// 反復中なので即時生成せず、遅延生成コマンドに積む
		for (auto& _data : _instanceVec)
		{
			a_world.ReserveCreateEntityWithData(_data.sig, std::move(_data.dataMap));
		}
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

	//======================================================================================
	// 即時生成
	//======================================================================================
	bool BuildSpawnInstanceData(
		Engine::ECS::World& a_world,
		const Engine::Resource::Prefab& a_prefab,
		const SpawnParams& a_params,
		std::vector<Engine::Resource::PrefabInstanceData>& a_outInstanceVec)
	{
		//------------------------------------------------------------------
		// ルート + 子ぶんの材料を作る
		//------------------------------------------------------------------
		// GUID の振り直しと親子リンクの張り替えはプレハブ側で済んでいる。
		// 先頭がルートで、親が子より前に並んでいる。
		// 子は保存された姿のまま出す(位置と向きは親に追従する)ので、上書きはルートだけ
		//------------------------------------------------------------------
		a_outInstanceVec = a_prefab.BuildInstanceData(&a_world);
		if (a_outInstanceVec.empty()) return false;

		Engine::Resource::PrefabInstanceData& _root = a_outInstanceVec[0];

		// 位置と向きを入れる。LocalTransform が無ければ足す
		if (uint8_t* _pBuf = EnsureInstanceComponent(
			a_world, _root, a_world.GetCompTypeID<LocalTransformComponent>()))
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
			if (uint8_t* _pBuf = EnsureInstanceComponent(
				a_world, _root, a_world.GetCompTypeID<SpawnerComponent>()))
			{
				SpawnerComponent _spawner = {};
				std::memcpy(&_spawner, _pBuf, sizeof(_spawner));
				_spawner.spawnerGUID = a_params.spawnerGUID;
				_spawner.waveIndex   = a_params.waveIndex;
				std::memcpy(_pBuf, &_spawner, sizeof(_spawner));
			}
		}

		// 追従先。FollowTargetLinkSystem(Awake)が GUID から引き直すので GUID も同じ相手に揃える
		if (a_params.followTarget != Engine::ECS::Limits::INVALID_ENTITY)
		{
			if (uint8_t* _pBuf = EnsureInstanceComponent(
				a_world, _root, a_world.GetCompTypeID<FollowTargetComponent>()))
			{
				FollowTargetComponent _follow = {};
				std::memcpy(&_follow, _pBuf, sizeof(_follow));
				_follow.target     = a_params.followTarget;
				_follow.targetGUID = a_params.followTargetGUID;
				std::memcpy(_pBuf, &_follow, sizeof(_follow));
			}
		}

		return true;
	}

	uint8_t* EnsureInstanceComponent(
		Engine::ECS::World& a_world,
		Engine::Resource::PrefabInstanceData& a_data,
		Engine::ECS::ComponentTypeID a_typeID)
	{
		return EnsureComponentBuffer(a_world, a_data.sig, a_data.dataMap, a_typeID);
	}

	Engine::ECS::Entity CreateInstanceNow(
		Engine::ECS::World& a_world,
		std::vector<Engine::Resource::PrefabInstanceData>& a_instanceVec)
	{
		Engine::ECS::Entity _rootEntity = Engine::ECS::Limits::INVALID_ENTITY;

		for (size_t _i = 0; _i < a_instanceVec.size(); ++_i)
		{
			Engine::Resource::PrefabInstanceData& _data = a_instanceVec[_i];

			// シグネチャで実体を生成(各コンポーネントは既定構築される)
			const Engine::ECS::Entity _entity = a_world.CreateEntity(_data.sig);
			if (_entity == Engine::ECS::Limits::INVALID_ENTITY) continue;

			if (_i == 0) _rootEntity = _entity;

			// 既定構築済みのところへ、材料の初期値を上書きする
			// (親子リンクは parentGUID を張り替えてあるので HierarchyLinkSystem が繋ぎ直す)
			for (const auto& [_typeID, _buffer] : _data.dataMap)
			{
				if (_buffer.empty()) continue;
				if (!_data.sig.test(_typeID)) continue;

				uint8_t* _pDst = a_world.NRefData(_entity, _typeID);
				if (!_pDst) continue;

				const size_t _size = a_world.GetComponentMetaData(_typeID).compSize;
				std::memcpy(_pDst, _buffer.data(), _size);
			}
		}

		return _rootEntity;
	}
}
