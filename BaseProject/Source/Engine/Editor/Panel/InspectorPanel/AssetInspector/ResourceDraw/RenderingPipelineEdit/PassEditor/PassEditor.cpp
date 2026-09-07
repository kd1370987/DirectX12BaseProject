#include "PassEditor.h"

#include "Engine/Graphics/RenderingPipeline/Core/Pass/Pass.h"

namespace Engine::Editor::Inspector
{
	IPassEditor* PassEditorRegistry::Find(const Graphics::Pipeline::Pass& a_pass) const
	{
		// 実体の型で引く。Pass は仮想関数を持つので typeid は派生の型を返す
		auto _it = m_editorMap.find(std::type_index(typeid(a_pass)));
		if (_it == m_editorMap.end()) return nullptr;

		return _it->second.get();
	}
}
