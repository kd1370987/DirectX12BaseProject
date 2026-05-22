#include "StateMachineAsset.h"

UINT Engine::Resource::StateMachineAsset::GetStateHash(const std::string& a_stateName)
{
	UINT _hash = StringUtility::ToHash(a_stateName);

	auto _it = m_stateNodeMap.find(_hash);
	if (_it != m_stateNodeMap.end())
	{
		return _it->first;
	}
	return -1;
}
