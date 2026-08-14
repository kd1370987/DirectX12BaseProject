#include "PrefabSpawnHelper.h"

#include <cstring>

#include "Engine/ECS/World/World.h"
#include "Engine/Resource/Manager/ResourceManager/ResourceManager.h"
#include "Engine/Resource/Data/Prefab/Prefab.h"

#include "../Components/Transform/LocalTransformComponent.h"

namespace App::Utility
{
	void SpawnPrefabAt(
		Engine::ECS::World& a_world,
		Engine::Resource::ResourceManager& a_resourceManager,
		const Engine::GUID& a_prefabGUID,
		Engine::Handle<Engine::Resource::Prefab>& a_refHandle,
		const DirectX::XMFLOAT3& a_pos)
	{
		if (a_prefabGUID == Engine::DefaultGUID) return;

		// ハンドルを解決(未ロードならロード)
		if (!a_resourceManager.IsValid(a_refHandle))
		{
			if (!a_resourceManager.Has<Engine::Resource::Prefab>(a_prefabGUID))
			{
				a_resourceManager.LoadImmediate<Engine::Resource::Prefab>(a_prefabGUID);
			}
			a_refHandle = a_resourceManager.GetCache<Engine::Resource::Prefab>(a_prefabGUID);
		}

		auto* _pPrefab = a_resourceManager.Ref(a_refHandle);
		if (!_pPrefab) return;

		Engine::ECS::Signature _sig = _pPrefab->GetSignature();
		auto _data = _pPrefab->GetDataMap();	// コピー(型ID -> バイト列)

		auto _ltID = a_world.GetCompTypeID<LocalTransformComponent>();

		// 位置を入れるため LocalTransform が無ければ足す
		if (!_sig.test(_ltID))
		{
			_sig.set(_ltID);
			auto& _buf = _data[_ltID];
			_buf.assign(a_world.GetComponentMetaData(_ltID).compAlignSize, 0);
			auto _ctor = a_world.GetCompFunc(_ltID).construct;
			if (_ctor) _ctor(_buf.data());
		}

		// 指定座標に配置
		{
			auto& _buf = _data[_ltID];
			LocalTransformComponent _lt = {};
			std::memcpy(&_lt, _buf.data(), sizeof(_lt));
			_lt.pos = a_pos;
			_lt.isDirty = true;
			std::memcpy(_buf.data(), &_lt, sizeof(_lt));
		}

		// 反復中なので即時生成せず、遅延生成コマンドに積む
		a_world.AddEntityWithData(_sig, std::move(_data));
	}
}
