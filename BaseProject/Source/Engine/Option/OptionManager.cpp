#include "OptionManager.h"
namespace Engine::Option
{
	void OptionManager::Serialize()
	{
		Persistence::Archive _archive(Persistence::Archive::Mode::Save,"Asset/Data/Option","EngineOption","optn");
		m_giOptions.Archive(_archive);

		Persistence::Archive _winArchive(
			Persistence::Archive::Mode::Save,
			"Asset/Data/Option/Graphics",
			"WindowOption",
			"optn"
		);
		m_windowOption.Archive(_winArchive);
	}
	void OptionManager::Deserialize()
	{
		Persistence::Archive _archive(Persistence::Archive::Mode::Load, "Asset/Data/Option", "EngineOption", "optn");
		m_giOptions.Archive(_archive);

		Persistence::Archive _winArchive(
			Persistence::Archive::Mode::Load, 
			"Asset/Data/Option/Graphics", 
			"WindowOption", 
			"optn",
			Persistence::Archive::ArchiveFormat::Json
		);
		m_windowOption.Archive(_winArchive);

	}
	void Engine::Option::OptionManager::DrawEdit()
	{
		m_giOptions.DrawEdit();
		m_windowOption.DrawEdit();
	}
}