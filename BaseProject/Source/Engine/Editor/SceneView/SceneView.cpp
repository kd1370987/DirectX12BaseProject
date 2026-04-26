#include "SceneView.h"

#include "EditorCamera/EditorCamera.h"

namespace Engine::Editor
{
	void Engine::Editor::SceneView::Init()
	{
		m_upEditorCamera = std::make_unique<EditorCamera>();
		m_upEditorCamera->Init(1280,720);
	}
	void SceneView::Update()
	{
		m_upEditorCamera->Update();
	}
	const EditorCamera* SceneView::GetEditorCamera()
	{
		if (!m_upEditorCamera) return nullptr;

		return m_upEditorCamera.get();
	}
}
