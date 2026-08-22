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
		m_pOptionList.push_back(&m_bloomOption);
		m_pOptionList.push_back(&m_toneMapOption);

		m_pOptionList.push_back(&m_buildConfig);
		m_pOptionList.push_back(&m_inputOption);
		m_pOptionList.push_back(&m_cursorOption);

		m_pOptionList.push_back(&m_debugDrawOption);
	}
	void OptionManager::Serialize()
	{
		Persistence::Archive _archive(Persistence::Archive::Mode::Save, "Asset/Data/Engine", "EngineData", "optn");
		Archive(_archive);
	}
	//======================================================================================
	// エンジン設定の読み込み
	//
	// ここだけは形式を JSON で固定している。ビルドモード(Development / Shipping)を
	// 決めているのがこのファイル自身で、読む時点ではまだモードが分からないため。
	// Auto にすると「モードを決めるためにモードを見る」ことになる。
	//======================================================================================
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