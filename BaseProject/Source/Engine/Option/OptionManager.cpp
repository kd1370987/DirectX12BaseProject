#include "OptionManager.h"


namespace Engine::Option
{
	OptionManager::OptionManager(){}
	void OptionManager::Init()
	{
		m_pOptionList.clear();
		m_pOptionList.push_back(&m_giOptions);
		m_pOptionList.push_back(&m_windowOption);
		m_pOptionList.push_back(&m_renderingOption);
		m_pOptionList.push_back(&m_lightingOption);

		m_pOptionList.push_back(&m_buildConfig);
		m_pOptionList.push_back(&m_inputOption);
	}
	void OptionManager::Serialize()
	{
		Persistence::Archive _archive(Persistence::Archive::Mode::Save, "Asset/Data/Engine", "EngineData", "optn");
		Archive(_archive);
	}
	void OptionManager::Deserialize()
	{
		Persistence::Archive _archive(
			Persistence::Archive::Mode::Load, 
			"Asset/Data/Engine",
			"EngineData", 
			"optn",
			Persistence::Archive::ArchiveFormat::Json
		);
		Archive(_archive);
	}
	void Engine::Option::OptionManager::DrawEdit()
	{
		for (auto* _pOption : m_pOptionList)
		{
			ImGui::Separator();
			if (ImGui::TreeNodeEx(_pOption->GetName().c_str(), ImGuiTreeNodeFlags_SpanFullWidth))
			{
				_pOption->DrawEdit();
				ImGui::TreePop();
			}
		}
	}
	void OptionManager::Archive(Persistence::Archive& a_ar)
	{
		for (auto* _pOption : m_pOptionList)
		{
			if (a_ar.BeginGroup(_pOption->GetName().c_str()))
			{
				_pOption->Archive(a_ar);
				a_ar.EndGroup();
			}
		}
	}
}