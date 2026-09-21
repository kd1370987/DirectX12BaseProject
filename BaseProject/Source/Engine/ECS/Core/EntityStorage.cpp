#include "EntityStorage.h"

#include "../Core/Chunk.h"
#include "../ComponentMetaRegistry/ComponentMetaRegistry.h"

namespace Engine::ECS
{
	void EntityStorage::Init(ComponentMetaRegistry* a_pRegistry, const EngineServices* a_pServices)
	{
		m_pRegistry = a_pRegistry;
		m_pServices = a_pServices;

		m_entityManager.Init();
		m_archetypeManager.Init(a_pRegistry);
	}

	//======================================================================================
	// 生成
	//======================================================================================
	Entity EntityStorage::Create(const Signature& a_sig)
	{
		const Entity _entity = m_entityManager.CreateEntity(a_sig);
		if (_entity == Limits::INVALID_ENTITY) return Limits::INVALID_ENTITY;

		AttachToChunk(_entity, a_sig);

		// シグネチャごとにコンストラクタを回す
		for (ComponentTypeID _compID = 0; _compID < a_sig.size(); ++_compID)
		{
			if (!a_sig.test(_compID)) continue;
			m_pRegistry->GetFunc(_compID).construct(RefComponent(_entity, _compID));
		}

		return _entity;
	}

	//======================================================================================
	// 削除
	//======================================================================================
	void EntityStorage::Destroy(const Entity& a_entity)
	{
		// 住所は値で持つ。DestroyEntity で元の住所は空に戻される
		const EntityLocation _loca = m_entityManager.GetLocation(a_entity);
		if (!_loca.pChunk) return;

		// 消える前に、コンポーネントが借りているものを返させる。
		// コンポーネントはデストラクタが走らない(trivially copyable 縛り)ので、
		// リソースの参照カウントはここで返さないと戻らない
		ReleaseComponents(a_entity, m_entityManager.GetSignature(a_entity));

		DetachFromChunk(_loca);
		m_entityManager.DestroyEntity(a_entity);
	}

	//======================================================================================
	// 引っ越し
	//--------------------------------------------------------------------------------------
	// 退避 → 返却 → 旧チャンクから抜く → 新チャンクへ載せる → 書き戻し → 初期値で上書き
	//======================================================================================
	void EntityStorage::Move(const Entity& a_entity, const Signature& a_toSig, const ComponentDataMap& a_initData, bool a_isReleaseAll)
	{
		// 引っ越し前の状態は値で持つ。
		// 参照で持つと、途中の SetSignature / SetEntityLocation で新しい値に書き換わり、
		// 後半の「元から持っていたか」の判定が意味を失う
		const Signature _oldSig = m_entityManager.GetSignature(a_entity);
		const EntityLocation _oldLoca = m_entityManager.GetLocation(a_entity);

		ComponentDataMap _snapshot = SnapshotComponents(a_entity, _oldSig);
		ReleaseBeforeMove(_snapshot, a_toSig, a_initData, a_isReleaseAll);

		DetachFromChunk(_oldLoca);
		AttachToChunk(a_entity, a_toSig);

		RestoreComponents(a_entity, a_toSig, _snapshot);
		WriteComponentData(a_entity, a_initData);
	}

	void EntityStorage::WriteComponentData(const Entity& a_entity, const ComponentDataMap& a_dataMap)
	{
		const Signature& _sig = m_entityManager.GetSignature(a_entity);

		for (const auto& [_compID, _buffer] : a_dataMap)
		{
			// 書く場所が無いもの・中身が無いものは飛ばす
			if (!_sig.test(_compID)) continue;
			if (_buffer.empty()) continue;

			uint8_t* _pData = RefComponent(a_entity, _compID);
			if (!_pData) continue;

			// バッファが短いときに読み越さない
			const size_t _size = m_pRegistry->GetMetaData(_compID).compSize;
			const size_t _copy = (_size < _buffer.size()) ? _size : _buffer.size();
			std::memcpy(_pData, _buffer.data(), _copy);
		}
	}

	uint8_t* EntityStorage::RefComponent(const Entity& a_entity, ComponentTypeID a_typeID)
	{
		return m_archetypeManager.RefComponent(m_entityManager.GetLocation(a_entity), a_typeID);
	}

	//======================================================================================
	// 内部処理
	//======================================================================================
	void EntityStorage::AttachToChunk(const Entity& a_entity, const Signature& a_sig)
	{
		const EntityLocation _loca = m_archetypeManager.AllocationEntity(a_entity, a_sig);
		m_entityManager.SetEntityLocation(a_entity, _loca);
		m_entityManager.SetSignature(a_entity, a_sig);
	}

	void EntityStorage::DetachFromChunk(const EntityLocation& a_location)
	{
		// アーキタイプから抜いて、穴へ詰めたエンティティの情報をもらう
		auto [_movedEntity, _movedIdx] = m_archetypeManager.RemoveEntity(a_location);

		// 末尾を抜いたときは誰も動いていない
		if (_movedEntity != Limits::INVALID_ENTITY)
		{
			m_entityManager.RefEntityLocation(_movedEntity).chunkIndex = _movedIdx;
		}
	}

	ComponentDataMap EntityStorage::SnapshotComponents(const Entity& a_entity, const Signature& a_sig)
	{
		ComponentDataMap _snapshot = {};

		for (ComponentTypeID _compID = 0; _compID < a_sig.size(); ++_compID)
		{
			if (!a_sig.test(_compID)) continue;

			const size_t _size = m_pRegistry->GetMetaData(_compID).compSize;
			const uint8_t* _pData = RefComponent(a_entity, _compID);

			_snapshot[_compID] = std::vector<uint8_t>(_pData, _pData + _size);
		}
		return _snapshot;
	}

	//--------------------------------------------------------------------------------------
	// 借りているものを返させる
	//
	// 対象は次の3つ。どれも退避したバッファに対して呼ぶので、
	// ハンドルを空にした結果は引っ越し先へそのまま伝わる。
	//
	//   ・外されるコンポーネント       : この先持ち主がいなくなる
	//   ・初期値で上書きされるもの     : 今持っているぶんが宙に浮く
	//   ・初期化をやり直すエンティティ : 直後に取り直されるので、返さないと二重に持つ
	//--------------------------------------------------------------------------------------
	void EntityStorage::ReleaseBeforeMove(ComponentDataMap& a_snapshot, const Signature& a_toSig, const ComponentDataMap& a_initData, bool a_isReleaseAll)
	{
		for (auto& [_compID, _buffer] : a_snapshot)
		{
			const bool _isRemoved = !a_toSig.test(_compID);
			const bool _isOverwritten = a_initData.contains(_compID);

			if (!_isRemoved && !_isOverwritten && !a_isReleaseAll) continue;

			ReleaseComponentData(_compID, _buffer.data());
		}
	}

	void EntityStorage::RestoreComponents(const Entity& a_entity, const Signature& a_toSig, const ComponentDataMap& a_snapshot)
	{
		for (const auto& [_compID, _buffer] : a_snapshot)
		{
			// 引っ越し先に無いものは書く場所が無い
			if (!a_toSig.test(_compID)) continue;

			if (uint8_t* _pData = RefComponent(a_entity, _compID))
			{
				std::memcpy(_pData, _buffer.data(), _buffer.size());
			}
		}
	}

	//--------------------------------------------------------------------------------------
	// ComponentTraits<T>::Release を書いてあるコンポーネントだけが対象。
	// 解放フックはハンドルを空にするので、返したものを持ち主のふりで持ち続けない。
	//--------------------------------------------------------------------------------------
	void EntityStorage::ReleaseComponents(const Entity& a_entity, const Signature& a_sig)
	{
		for (ComponentTypeID _compID = 0; _compID < a_sig.size(); ++_compID)
		{
			if (!a_sig.test(_compID)) continue;
			ReleaseComponentData(_compID, RefComponent(a_entity, _compID));
		}
	}

	void EntityStorage::ReleaseComponentData(ComponentTypeID a_compID, uint8_t* a_pData)
	{
		if (!a_pData) return;

		const auto& _release = m_pRegistry->GetFunc(a_compID).release;
		if (!_release) return;

		_release(a_pData, *m_pServices);
	}
}
