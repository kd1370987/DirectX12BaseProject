#include "SystemManager.h"

void SystemManager::Init()
{
	for (UINT _i = 0; _i < (UINT)SystemType::Num; ++_i)
	{
		m_systemMap.emplace((SystemType)_i, std::vector<std::shared_ptr<ISystem>>{});
	}
}

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
