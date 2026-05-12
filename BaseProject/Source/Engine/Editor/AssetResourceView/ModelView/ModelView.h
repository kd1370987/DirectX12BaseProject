#pragma once
namespace Engine::Editor
{
	class ModelView
	{
	public:

		void DrawModel(const Engine::GUID& a_guid);

	private:

		void DrawModelView(const Engine::Resource::Model& a_model);

		void NodeView(const Engine::Resource::Node& a_node);

	
		void AnimationView(const Engine::Resource::Model& a_model);
	};
}