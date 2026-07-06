#pragma once

namespace Engine
{
	namespace ECS
	{
		class World;
	}
}

namespace Engine::Editor
{
	class EditorCamera;
	

	class SceneView
	{
	public:

		void Init();

		void Update(float a_dt);

		const EditorCamera* GetEditorCamera();

		static void Draw(const ECS::Entity& a_currentSelectEntity, Engine::ECS::World* a_pWorld);

	private:

		static void GuizmoDraw(const ImVec2& a_pos, const ImVec2& a_rect,const ECS::Entity& a_currentSelectEntity, Engine::ECS::World* a_pWorld);

	private:

		std::unique_ptr<EditorCamera> m_upEditorCamera = nullptr;

	};
}