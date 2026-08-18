#include "GameObjectManager.h"

#include "../../ECS/World/World.h"

namespace Engine::GameObject
{
	GameObjectManager::GameObjectManager(Engine::ECS::World* a_pWorld)
	{
		// 以降 Init/Update/Draw/Archive へ渡すコンテキストに載せておく。
		// サービス群はワールドが持っているものをそのまま使う
		// (シーン側で組んだ1つの束を、システムとオブジェクトで共有する)
		m_objContext.pWorld = a_pWorld;
		m_objContext.pServices = a_pWorld ? a_pWorld->RefEngineServices() : nullptr;

		// オブジェクトから他のオブジェクトを引けるようにする
		m_objContext.pObjectManager = this;
	}
	GameObjectManager::~GameObjectManager()
	{
		// 借りているもの(サウンドインスタンス等)を返してから破棄する。
		// プールはアプリ寿命なので、返さないとシーンを跨いで溜まり続ける
		for (auto& _upObject : m_upObjectVec)
		{
			if (_upObject) _upObject->Release(m_objContext);
		}
	}
	void GameObjectManager::PreUpdate()
	{
		// 削除処理 : オブジェクト自身が削除命令を出した場合に配列上から削除する
		for (size_t _idx = 0; _idx < m_upObjectVec.size(); ++_idx)
		{
			if (m_upObjectVec[_idx]->IsExpired())
			{
				// 消える前に借りているものを返させる
				m_upObjectVec[_idx]->Release(m_objContext);

				// GUID対応表からも取り除く
				m_guidMap.erase(m_upObjectVec[_idx]->GetGUID());

				std::swap(m_upObjectVec[_idx],m_upObjectVec.back());
				m_upObjectVec.pop_back();
				--_idx;	// swap してきた要素を再チェックする
			}
		}
	}
	void GameObjectManager::Update(float a_dt)
	{
		m_objContext.dt = a_dt;

		for (auto& _object : m_upObjectVec)
		{
			_object->Update(m_objContext);
		}
	}

	void GameObjectManager::Draw(float a_dt)
	{
		m_objContext.dt = a_dt;

		for (auto& _object : m_upObjectVec)
		{
			_object->Draw(m_objContext);
		}
	}

	BaseObject* GameObjectManager::Register(std::unique_ptr<BaseObject> a_upObject)
	{
		if (!a_upObject) return nullptr;

		BaseObject* _pObject = a_upObject.get();

		// GUIDが有効なら対応表へ登録
		if (_pObject->GetGUID().IsValid())
		{
			m_guidMap[_pObject->GetGUID()] = _pObject;
		}

		m_upObjectVec.push_back(std::move(a_upObject));
		return _pObject;
	}

	BaseObject* GameObjectManager::AddObjectByTypeID(ObjectTypeID a_typeID)
	{
		// クラスメタマネージャーからファクトリで生成
		auto _upObject = ObjectMetaRegistry::Instance().Create(a_typeID);
		if (!_upObject)
		{
			return nullptr;
		}

		// 新規GUIDを発行
		Engine::GUID _guid = {};
		_guid.Create();
		_upObject->SetGUID(_guid);

		// 追加して初期化
		BaseObject* _pObject = Register(std::move(_upObject));
		_pObject->Init(m_objContext);
		return _pObject;
	}

	BaseObject* GameObjectManager::FindByGUID(const Engine::GUID& a_guid) const
	{
		auto _it = m_guidMap.find(a_guid);
		if (_it != m_guidMap.end())
		{
			return _it->second;
		}
		return nullptr;
	}

	void GameObjectManager::Archive(Persistence::Archive& a_ar)
	{
		auto& _registry = ObjectMetaRegistry::Instance();

		// ------------------------------------------------------------------
		// 配列サイズ : 保存時は現在の個数、読み込み時はファイルから取得
		// ------------------------------------------------------------------
		size_t _count = m_upObjectVec.size();
		if (a_ar.BeginArray("GameObjects", _count))
		{
			for (size_t _i = 0; _i < _count; ++_i)
			{
				if (!a_ar.BeginObject(_i)) continue;

				if (a_ar.IsSaving())
				{
					// --------------------------------------------------
					// 保存 : タイプインデックス / GUID / データ を書き出す
					// --------------------------------------------------
					BaseObject* _pObject = m_upObjectVec[_i].get();

					// C++型から登録済みタイプIDを引く
					ObjectTypeID _typeID = _registry.GetTypeID(std::type_index(typeid(*_pObject)));

					// GUIDが未発行なら発行しておく
					Engine::GUID _guid = _pObject->GetGUID();
					if (!_guid.IsValid())
					{
						_guid.Create();
						_pObject->SetGUID(_guid);
					}

					a_ar.Field("TypeIndex", _typeID);
					a_ar.GUIDField("GUID", _guid);

					// 個別データ
					if (a_ar.BeginGroup("Data"))
					{
						_pObject->Archive(a_ar, m_objContext);
						a_ar.EndGroup();
					}
				}
				else
				{
					// --------------------------------------------------
					// 読み込み : タイプインデックスからクラスを復元する
					// --------------------------------------------------
					ObjectTypeID _typeID = INVALID_OBJECT_TYPE_ID;
					Engine::GUID _guid = {};
					a_ar.Field("TypeIndex", _typeID);
					a_ar.GUIDField("GUID", _guid);

					// ファクトリで実体を生成
					auto _upObject = _registry.Create(_typeID);
					if (_upObject)
					{
						// GUIDを復元して登録
						_upObject->SetGUID(_guid);
						BaseObject* _pObject = Register(std::move(_upObject));

						// 既定初期化 → 保存データで上書き復元
						_pObject->Init(m_objContext);
						if (a_ar.BeginGroup("Data"))
						{
							_pObject->Archive(a_ar, m_objContext);
							a_ar.EndGroup();
						}
					}
				}

				a_ar.EndObject();
			}
			a_ar.EndArray();
		}
	}
}
