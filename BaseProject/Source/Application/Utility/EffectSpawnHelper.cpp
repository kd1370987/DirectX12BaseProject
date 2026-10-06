#include "EffectSpawnHelper.h"

#include <cstring>

#include "Engine/ECS/World/World.h"

#include "../../Engine/MainEngine.h"
#include "../../Engine/Graphics/GraphicsEngine.h"
#include "../../Engine/Graphics/Particle/ParticleBufferManager.h"
#include "../../Engine/Graphics/Particle/GPU/EmitterSlotPool/EmitterSlotPool.h"
#include "../../Engine/Graphics/LightManager/LightManager.h"
#include "../../Engine/Effect/EffectInstance.h"


#include "Application/Components/Transform/LocalTransformComponent.h"
#include "Application/Components/Transform/WorldMatrixComponent.h"
#include "Application/Components/Effect/EffectAssetComponent.h"
#include "Application/Components/Effect/EffectOverrideComponent.h"
#include "Application/Components/Effect/EffectComponent.h"

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

		using EffectDataMap = std::unordered_map<Engine::ECS::ComponentTypeID, std::vector<uint8_t>>;

		//----------------------------------------------------------------------
		// エフェクト1体ぶんのシグネチャと初期データを組む。
		// 「エフェクトのエンティティとは何で出来ているか」をここ1か所に閉じておき、
		// 遅延生成(SpawnEffectAt)と即時生成(SpawnEffectAtNow)で同じものを使う。
		//----------------------------------------------------------------------
		bool BuildEffectEntity(
			Engine::ECS::World& a_world,
			const Engine::GUID& a_effectGUID,
			const Math::Vector3& a_pos,
			bool a_isDestroyOnFinish,
			const Math::Vector3& a_emitDir,
			float a_scale,
			Engine::ECS::Signature& a_outSig,
			EffectDataMap& a_outData)
		{
			Engine::ECS::Signature& _sig = a_outSig;
			EffectDataMap& _data = a_outData;

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

			if (!PushComponent(a_world, _sig, _data, _effect)) return false;

			// ---- 出す側からの上書き ----
			// アセットは共有なので、大きさと向きの違いはここで付ける。
			// このエンティティは平行移動しか持たないので、渡された向きはそのままワールドの向きになる
			EffectOverrideComponent _override = {};
			_override.effectScale = (a_scale > 0.0f) ? a_scale : 1.0f;

			Math::Vector3 _emitDir = a_emitDir;
			if (_emitDir.LengthSquared() > 1e-8f)
			{
				// 手で入れた値は長さがまちまちなので揃えておく。
				// 揃えないと受け側(EffectDrawSystem)で行列を掛けたときに長さが効いてしまう
				_emitDir.Normalize();
				_override.isOverrideTransform = true;
				_override.overrideEmitDir = _emitDir;
			}

			if (!PushComponent(a_world, _sig, _data, _override)) return false;

			// ---- エフェクトである印 ----
			// 一括停止などで絞り込めるように、既存のエフェクトと同じ目印を付けておく
			PushComponent(a_world, _sig, _data, EffectComponent{});

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
		EffectDataMap _data = {};

		if (!BuildEffectEntity(
			a_world, a_effectGUID, a_pos, a_isDestroyOnFinish, a_emitDir, a_scale, _sig, _data))
		{
			return false;
		}

		// 反復中なので即時生成せず、遅延生成コマンドに積む
		a_world.ReserveCreateEntityWithData(_sig, std::move(_data));
		return true;
	}

	//==========================================================================================
	// 即時生成
	//
	// 遅延生成だと ID が返らないので、出したあとも動かし続けたい相手はこちらで作る。
	// CreateEntity はコンストラクタまで回してくれるので、
	// 組んでおいた初期データを上から流し込めばそれで出来上がり。
	//==========================================================================================
	Engine::ECS::Entity SpawnEffectAtNow(
		Engine::ECS::World& a_world,
		const Engine::GUID& a_effectGUID,
		const Math::Vector3& a_pos,
		bool a_isDestroyOnFinish,
		const Math::Vector3& a_emitDir,
		float a_scale)
	{
		if (a_effectGUID == Engine::DefaultGUID) return Engine::ECS::Limits::INVALID_ENTITY;

		Engine::ECS::Signature _sig = {};
		EffectDataMap _data = {};

		if (!BuildEffectEntity(
			a_world, a_effectGUID, a_pos, a_isDestroyOnFinish, a_emitDir, a_scale, _sig, _data))
		{
			return Engine::ECS::Limits::INVALID_ENTITY;
		}

		const Engine::ECS::Entity _entity = a_world.CreateEntity(_sig);
		if (_entity == Engine::ECS::Limits::INVALID_ENTITY) return _entity;

		// 既定構築済みのところへ、組んでおいた初期値を上書きする
		for (const auto& [_typeID, _buf] : _data)
		{
			uint8_t* _pDst = a_world.NRefData(_entity, _typeID);
			if (!_pDst || _buf.empty()) continue;

			std::memcpy(_pDst, _buf.data(), _buf.size());
		}

		return _entity;
	}
	void WarmupEffectParticles(const Engine::ECS::EngineServices& a_services, Engine::Handle<Engine::Resource::EffectAsset> a_effectHandle)
	{
		if (!a_services.pResourceManager || !a_services.pMainEngine)  return;

		auto* _pGE = a_services.pMainEngine->RefGraphicsEngine();
		if (!_pGE) return;

		auto* _pPM = _pGE->RefParticleManager();
		if (!_pPM) return;

		// エフェクト取得
		auto* _pEffect = a_services.pResourceManager->Get(a_effectHandle);
		if (!_pEffect) return;

		// エフェクトからとれるパーツのプールを作成する。
		// パーティクル未設定のパーツや、アセットがまだ読めていないものは
		// CreateParticleDataAsync の側で弾かれる(後で RequestEmit が作り直しに来る)
		for (auto& _part : _pEffect->GetParticleParts())
		{
			if (!_part.IsValid()) continue;
			_pPM->CreateParticleDataAsync(_part.particleHandle);
		}
	}
	void ReserveReturnEffectEmitterSlot(
		const Engine::ECS::EngineServices& a_services,
		Engine::Handle<Engine::Resource::EffectAsset> a_effectHandle,
		Engine::Handle<Engine::Particle::EmitterTransform>& a_emitterSlot)
	{
		if (!a_emitterSlot.IsValid()) return;

		// 手元は先に空にしておく(どこで抜けても二重に返さない)
		const auto _slot = a_emitterSlot;
		a_emitterSlot = {};

		if (!a_services.pMainEngine) return;
		auto* _pGE = a_services.pMainEngine->RefGraphicsEngine();
		if (!_pGE) return;
		auto* _pPM = _pGE->RefParticleManager();
		if (!_pPM) return;
		auto* _pSlotPool = _pPM->RefEmitterSlotPool();
		if (!_pSlotPool) return;

		//----------------------------------------------------------------------
		// 待つ時間 : このエフェクトでローカル空間で回すパーツ(パーツの上書き込み)の最大寿命
		//
		// 粒の寿命は [LifeTimeMin, LifeTimeMax] の乱数なので、Max だけ待てば全部消えている。
		// エフェクトやパーティクルが引けず寿命が分からないときは長めの代わりの値で待つ
		//----------------------------------------------------------------------
		float _holdSeconds = 0.0f;
		bool _isKnown = false;

		const auto* _pEffect = a_services.pResourceManager ? a_services.pResourceManager->Get(a_effectHandle) : nullptr;
		if (_pEffect)
		{
			_isKnown = true;
			for (const auto& _part : _pEffect->GetParticleParts())
			{
				if (!_part.IsValid()) continue;

				const auto* _pParticle = a_services.pResourceManager->Get(_part.particleHandle);
				if (!_pParticle)
				{
					_isKnown = false;
					continue;
				}
				if (!_part.IsLocalSimulation(_pParticle)) continue;

				_holdSeconds = (std::max)(_holdSeconds, _pParticle->GetLifeTimeMax());
			}
		}
		if (!_isKnown)
		{
			_holdSeconds = (std::max)(_holdSeconds, Engine::Particle::EMITTER_SLOT_FALLBACK_HOLD_SECONDS);
		}

		_pSlotPool->ReserveReturn(_slot, _holdSeconds);
	}
	void ReleaseEffectLights(
		const Engine::ECS::EngineServices& a_services,
		Engine::Effect::EffectInstance& a_instance)
	{
		// 終了処理の順によっては、こちらが先に消えていることがある(PointLightComponent と同じ)
		auto* _pGE = a_services.pMainEngine ? a_services.pMainEngine->RefGraphicsEngine() : nullptr;
		if (!_pGE || !_pGE->RefLightManager()) return;

		a_instance.ReleaseLights(*_pGE->RefLightManager());
	}
}
