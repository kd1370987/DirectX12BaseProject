#include "OptionManager.h"
namespace Engine::Option
{
	void OptionManager::Serialize()
	{
		Persistence::Archive _archive(Persistence::Archive::Mode::Save, "Asset/Data/Option", "GraphicsOption", "optn");
		Archive(_archive);
	}
	void OptionManager::Deserialize()
	{
		Persistence::Archive _archive(Persistence::Archive::Mode::Load, "Asset/Data/Option", "GraphicsOption", "optn");
		Archive(_archive);
	}
	void Engine::Option::OptionManager::DrawEdit()
	{
		m_giOptions.DrawEdit();
		m_windowOption.DrawEdit();
		m_renderingOption.DrawEdit();
	}
	void OptionManager::Archive(Persistence::Archive& a_ar)
	{
		if (a_ar.BeginGroup("RenderingOption"))
		{
			m_renderingOption.Archive(a_ar);
			a_ar.EndGroup();
		}
		if (a_ar.BeginGroup("WindowOption"))
		{
			m_windowOption.Archive(a_ar);
			a_ar.EndGroup();
		}
		if (a_ar.BeginGroup("GIOption"))
		{
			m_giOptions.Archive(a_ar);
			a_ar.EndGroup();
		}
	}
}