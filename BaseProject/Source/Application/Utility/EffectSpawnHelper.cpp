#include "EffectSpawnHelper.h"

#include <cstring>

#include "Engine/ECS/World/World.h"

#include "../Components/Transform/LocalTransformComponent.h"
#include "../Components/Transform/WorldMatrixComponent.h"
#include "../Components/Effect/EffectAssetComponent.h"
#include "../Components/Effect/EffectComponent.h"

namespace App::Utility
{
	namespace
	{
		//----------------------------------------------------------------------
		// シグネチャへ1コンポーネント足して、既定構築した書き込み先を返す。
		// ここは何も無いところから組み立てるので「すでに持っているか」は見ない。
		//----------------------------------------------------------------------
		template<typename Comp>
		bool PushComponent(
			Engine::ECS::World& a_world,
			Engine::ECS::Signature& a_sig,
			std::unordered_map<Engine::ECS::ComponentTypeID, std::vector<uint8_t>>& a_data,
			const Comp& a_value)
		{
			const auto _typeID = a_world.GetCompTypeID<Comp>();
			if (_typeID == Engine::ECS::Limits::INVALID_COMPONENTTYPEID) return false;

			a_sig.set(_typeID);

			// チャンク上の1件ぶんの大きさで確保する(アライン込み)
			auto& _buf = a_data[_typeID];
			_buf.assign(a_world.GetComponentMetaData(_typeID).compAlignSize, 0);
			if (_buf.empty()) return false;

			// 既定構築してから値を流し込む
			if (auto _ctor = a_world.GetCompFunc(_typeID).construct) _ctor(_buf.data());
			std::memcpy(_buf.data(), &a_value, sizeof(Comp));

			return true;
		}
	}

	bool SpawnEffectAt(
		Engine::ECS::World& a_world,
		const Engine::GUID& a_effectGUID,
		const Math::Vector3& a_pos,
		bool a_isDestroyOnFinish,
		const Math::Vector3& a_emitDir,
		float a_scale)
	{
		if (a_effectGUID == Engine::DefaultGUID) return false;

		Engine::ECS::Signature _sig = {};
		std::unordered_map<Engine::ECS::ComponentTypeID, std::vector<uint8_t>> _data = {};

		// ---- 位置 ----
		LocalTransformComponent _localTrs = {};
		_localTrs.pos = a_pos;
		_localTrs.isDirty = true;
		if (!PushComponent(a_world, _sig, _data, _localTrs)) return false;

		// ---- ワールド行列 ----
		// CalcMatrixSystem(PostUpdate)が Draw の前に組み直すが、
		// 出た最初のフレームから正しい位置で出したいので、ここで平行移動だけ入れておく
		WorldMatrixComponent _worldMat = {};
		_worldMat.worldMat = Math::Matrix::CreateTranslation(a_pos);
		_worldMat.wasUpdatedThisFrame = false;
		if (!PushComponent(a_world, _sig, _data, _worldMat)) return false;

		// ---- 再生するエフェクト ----
		EffectAssetComponent _effect = {};
		_effect.effectGUID = a_effectGUID;
		_effect.playOnStart = true;			// 出た瞬間から再生する
		_effect.destroyOnFinish = a_isDestroyOnFinish;	// 出し切ったら自分から消える

		// ---- 出す側からの上書き ----
		// アセットは共有なので、大きさと向きの違いはここで付ける。
		// このエンティティは平行移動しか持たないので、渡された向きはそのままワールドの向きになる
		_effect.effectScale = (a_scale > 0.0f) ? a_scale : 1.0f;

		Math::Vector3 _emitDir = a_emitDir;
		if (_emitDir.LengthSquared() > 1e-8f)
		{
			// 手で入れた値は長さがまちまちなので揃えておく。
			// 揃えないと受け側(EffectDrawSystem)で行列を掛けたときに長さが効いてしまう
			_emitDir.Normalize();
			_effect.isOverrideTransform = true;
			_effect.overrideEmitDir = _emitDir;
		}

		if (!PushComponent(a_world, _sig, _data, _effect)) return false;

		// ---- エフェクトである印 ----
		// 一括停止などで絞り込めるように、既存のエフェクトと同じ目印を付けておく
		PushComponent(a_world, _sig, _data, EffectComponent{});

		// 反復中なので即時生成せず、遅延生成コマンドに積む
		a_world.AddEntityWithData(_sig, std::move(_data));
		return true;
	}
}
