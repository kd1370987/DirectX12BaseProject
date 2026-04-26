#pragma once

namespace Engine::Editor
{
	class EditorCamera;

	class SceneView
	{
	public:

		void Init();

		void Update();

		const EditorCamera* GetEditorCamera();

	private:

		std::unique_ptr<EditorCamera> m_upEditorCamera = nullptr;

	};
}