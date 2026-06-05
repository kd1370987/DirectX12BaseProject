#include "OptionManager.h"
namespace Engine::Option
{
	void OptionManager::Serialize()
	{
		Persistence::Archive _archive(Persistence::Archive::Mode::Save,"Asset/Data/Option","EngineOption","optn");
		m_giOptions.Archive(_archive);
	}
	void OptionManager::Deserialize()
	{
		Persistence::Archive _archive(Persistence::Archive::Mode::Load, "Asset/Data/Option", "EngineOption", "optn");
		m_giOptions.Archive(_archive);
	}
	void Engine::Option::OptionManager::DrawEdit()
	{
		m_giOptions.DrawEdit();
	}
}