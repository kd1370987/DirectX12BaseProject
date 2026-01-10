#include "SystemManager.h"

void SystemManager::Register(std::unique_ptr<ISystem> a_upSystem)
{
	m_upSystemVec.push_back(std::move(a_upSystem));
}

void SystemManager::RunSystem(World& a_world, const SystemType& a_type, float a_dt)
{
	int _idx = static_cast<int>(a_type);

	/*for (auto* _system : m_pSystemGroupVec[_idx])
	{
		_system->Update(a_world,a_dt);
	}*/

	for (auto& _sys : m_upSystemVec)
	{
		_sys->Update(a_world,a_dt);
	}
}

void SystemManager::Sort()
{
}
