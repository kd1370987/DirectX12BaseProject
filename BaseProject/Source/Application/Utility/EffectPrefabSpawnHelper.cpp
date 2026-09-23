#include "EffectPrefabSpawnHelper.h"

#include <cstring>

#include "Engine/ECS/World/World.h"
#include "Engine/Resource/Data/Prefab/Prefab.h"
#include "Engine/Resource/Data/EffectPrefab/EffectPrefab.h"
#include "Engine/Resource/Manager/ResourceManager/ResourceManager.h"

#include "PrefabSpawnHelper.h"

#include "../Components/Transform/WorldMatrixComponent.h"
#include "../Components/Common/LifeTimeComponent.h"

namespace App::Utility
{
	bool SpawnEffectPrefab(
		Engine::ECS::World& a_world,
		const Engine::Resource::EffectPrefab& a_effectPrefab,
		const Math::Vector3& a_pos,
		const EffectPrefabEditFunc& a_edit)
	{
		// 位置の書き込みと LocalTransform の補充は通常のプレハブと同じ
		SpawnParams _params = {};
		_params.pos = a_pos;

		std::vector<Engine::Resource::PrefabInstanceData> _instanceVec = {};
		if (!BuildSpawnInstanceData(a_world, a_effectPrefab.GetPrefab(), _params, _instanceVec)) return false;

		//------------------------------------------------------------------
		// 寿命 : 全ノードに付け、アセットの寿命で頭打ちにする
		//
		// ノード側に短い寿命が入っていればそちらを残す(先に消したい部品のため)。
		// 0 以下・負(無期限)はアセットの寿命に置き換える
		//------------------------------------------------------------------
		const float _lifeTime = a_effectPrefab.GetLifeTime();
		const auto _lifeTypeID = a_world.GetCompTypeID<LifeTimeComponent>();

		for (Engine::Resource::PrefabInstanceData& _data : _instanceVec)
		{
			// 持っていなかったノードは既定値(1秒)で足されるので、それを「短い寿命」と取り違えない
			const bool _isAuthored = _data.sig.test(_lifeTypeID);

			uint8_t* _pBuf = EnsureInstanceComponent(a_world, _data, _lifeTypeID);
			if (!_pBuf) continue;

			LifeTimeComponent _life = {};
			std::memcpy(&_life, _pBuf, sizeof(_life));
			if (!_isAuthored || _life.value <= 0.0f || _life.value > _lifeTime)
			{
				_life.value = _lifeTime;
			}
			std::memcpy(_pBuf, &_life, sizeof(_life));
		}

		// ルートのワールド行列 : エフェクトもモデルもここから出るので、入れ忘れを補う
		EnsureInstanceComponent(a_world, _instanceVec[0], a_world.GetCompTypeID<WorldMatrixComponent>());

		if (a_edit) a_edit(a_world, _instanceVec);

		// 反復中でも出せるよう、遅延生成コマンドに積む
		for (Engine::Resource::PrefabInstanceData& _data : _instanceVec)
		{
			a_world.ReserveCreateEntityWithData(_data.sig, std::move(_data.dataMap));
		}
		return true;
	}

	bool SpawnEffectPrefab(
		Engine::ECS::World& a_world,
		Engine::Resource::ResourceManager& a_resourceManager,
		Engine::Handle<Engine::Resource::EffectPrefab> a_handle,
		const Math::Vector3& a_pos,
		const EffectPrefabEditFunc& a_edit)
	{
		const auto* _pEffectPrefab = a_resourceManager.Get(a_handle);
		if (!_pEffectPrefab) return false;

		return SpawnEffectPrefab(a_world, *_pEffectPrefab, a_pos, a_edit);
	}
}
