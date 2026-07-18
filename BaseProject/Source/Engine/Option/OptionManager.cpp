#include "OptionManager.h"

#include "../Editor/EditorUI/EditorUI.h"

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
	}
	void OptionManager::Serialize()
	{
		Persistence::Archive _archive(Persistence::Archive::Mode::Save, "Asset/Data/Option", "GraphicsOption", "optn");
		Archive(_archive);
	}
	void OptionManager::Deserialize()
	{
		Persistence::Archive _archive(
			Persistence::Archive::Mode::Load, 
			"Asset/Data/Option",
			"GraphicsOption", 
			"optn",
			Persistence::Archive::ArchiveFormat::Json
		);
		Archive(_archive);
	}
	void Engine::Option::OptionManager::DrawEdit()
	{
		for (auto* _pOption : m_pOptionList)
		{
			_pOption->DrawEdit();
		}

		// インクルードの関係上CPPに隠蔽したいもの
		// シェーディングモデル
		Handle<Resource::ShadingModelTable> _temp;
		Editor::UI::DrawAssetSelectCombo<Resource::ShadingModelTable>(
			"Change Shading Model",
			"ShadingModelTable",
			m_renderingOption.defaultShadingModelTable,
			_temp
		);
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
		if (a_ar.BeginGroup("LightingOption"))
		{
			m_lightingOption.Archive(a_ar);
			a_ar.EndGroup();
		}
	}
}