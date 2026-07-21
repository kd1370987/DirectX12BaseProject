#include "Prefab.h"

#include "../../../ECS/World/World.h"

namespace Engine::Resource
{
	Prefab::Prefab()
	{
		m_sigunature = {};
		m_dataMap.clear();
	}
	void Prefab::AddComponent(ECS::ComponentTypeID a_compTypeID, uint8_t* a_pData)
	{
		if (m_sigunature.test(a_compTypeID))
		{
			m_sigunature.set(a_compTypeID);
		}
		// データはコピーして保持
		m_dataMap[a_compTypeID] = *a_pData;
	}

	void Prefab::RemoveComponent(ECS::ComponentTypeID a_compTypeID)
	{
		if (m_sigunature.test(a_compTypeID))
		{
			m_sigunature.reset(a_compTypeID);
		}

		auto _it = m_dataMap.find(a_compTypeID);
		if (_it != m_dataMap.end())
		{
			m_dataMap.erase(a_compTypeID);
		}
	}

	void Prefab::Archive(Persistence::Archive& a_ar, ECS::World* a_pWorld)
	{
		std::vector<std::string> _compNames = {};


		// 【セーブ時のみ】エンティティからコンポーネント名リストを作成
		if (a_ar.GetMode() == Persistence::Archive::Mode::Save)
		{
			for (auto& [_typeID, _meta] : a_pWorld->GetAllComponentMetaData())
			{
				if (m_sigunature.test(_typeID))
				{
					_compNames.push_back(_meta.name);
				}
			}
		}

		// コンポーネント名リストのアーカイブ
		a_ar.VectorField("ComponentNames", _compNames);

		// 【ロード時のみ】読み込んだリストからシグネチャを作り、エンティティを生成
		if (a_ar.GetMode() == Persistence::Archive::Mode::Load)
		{
			for (const std::string& _name : _compNames)
			{
				ECS::ComponentTypeID _typeID = a_pWorld->GetCompTypeID(_name);
				if (_typeID != ECS::Limits::INVALID_COMPONENTTYPEID)
				{
					m_sigunature.set(_typeID);
				}
			}
		}

		// ---------------------------------------------------------
		// 各コンポーネントデータのシリアライズ
		// ---------------------------------------------------------
		for (const std::string& _name : _compNames)
		{
			ECS::ComponentTypeID _typeID = a_pWorld->GetCompTypeID(_name);
			if (_typeID == ECS::Limits::INVALID_COMPONENTTYPEID) continue;

			auto _it = m_dataMap.find(_typeID);
			if (_it == m_dataMap.end()) continue;

			auto _func = a_pWorld->GetCompFunc(_typeID).archive;
			if (_func)
			{
				// セーブもロードも同じグループ構造で実行
				if (a_ar.BeginGroup(_name))
				{
					_func(a_ar, &_it->second);
					a_ar.EndGroup();
				}
			}
		}

	}
}