#include "SystemManager.h"

void SystemManager::RunSystem(World& a_world, const SystemType& a_type, float a_dt)
{
	int _idx = static_cast<int>(a_type);

	auto _it = m_systemMap.find(a_type);
	if (_it != m_systemMap.end())
	{
		for (auto& _system : _it->second)
		{
			_system->Update(a_world,a_dt);
		}
	}
}

void SystemManager::Sort()
{
}
