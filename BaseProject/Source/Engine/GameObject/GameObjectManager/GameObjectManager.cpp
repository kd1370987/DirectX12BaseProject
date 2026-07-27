#include "GameObjectManager.h"
namespace Engine::GameObject
{
	GameObjectManager::GameObjectManager()
	{}
	GameObjectManager::~GameObjectManager()
	{}
	void GameObjectManager::PreUpdate()
	{
		// 削除処理 : オブジェクト自身が削除命令を出した場合に配列上から削除する
		for (size_t _idx = 0; _idx < m_upObjectVec.size(); ++_idx)
		{
			if (m_upObjectVec[_idx]->IsExpired())
			{
				std::swap(m_upObjectVec[_idx],m_upObjectVec.back());
				m_upObjectVec.pop_back();
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
}