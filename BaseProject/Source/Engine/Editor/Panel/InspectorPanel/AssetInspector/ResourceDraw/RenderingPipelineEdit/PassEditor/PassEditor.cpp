#include "PassEditor.h"

#include "Engine/Graphics/RenderingPipeline/Core/Pass/Pass.h"

namespace Engine::Editor::Inspector
{
	IPassEditor* PassEditorRegistry::Find(const Graphics::Pipeline::Pass& a_pass) const
	{
		// 実体の型で引く(生成時に CreatePass が刻んだもの)
		auto _it = m_editorMap.find(a_pass.GetTypeChain()->key);
		if (_it == m_editorMap.end()) return nullptr;

		return _it->second.get();
	}
}
