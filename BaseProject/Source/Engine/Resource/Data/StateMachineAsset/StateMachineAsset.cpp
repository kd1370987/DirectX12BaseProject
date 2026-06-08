#include "StateMachineAsset.h"

UINT Engine::Resource::StateMachineAsset::GetStateHash(const std::string& a_stateName) const
{
	UINT _hash = StringUtility::ToHash(a_stateName);

	auto _it = m_stateNodeMap.find(_hash);
	if (_it != m_stateNodeMap.end())
	{
		return _it->first;
	}
	return -1;
}

void Engine::Resource::StateMachineAsset::Save(const std::string& a_savePath)
{	
}

void Engine::Resource::StateMachineAsset::Load(const std::string & a_filePath)
{

}

void Engine::Resource::StateMachineAsset::Release()
{
	m_stateNodeMap.clear();
	m_transitionArrowMap.clear();
}

void Engine::Resource::StateMachineAsset::EditImGui()
{

}
