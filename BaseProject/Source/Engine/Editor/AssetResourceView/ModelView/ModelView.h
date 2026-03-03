#pragma once

class ModelView
{
public:

	void Init();

	void Draw();

private:

	void DrawModelView(Engine::Resource::Model* a_pModel);

	void NodeView(Engine::Resource::Node& a_node);

	void MaterialView(Engine::Resource::Material& a_material);
	void MeshView(Engine::Resource::Mesh* a_pMesh);
	void AnimationView(Engine::Resource::AnimationData& a_animationData);
	void CollisionView(Engine::Resource::Mesh& a_pMesh);
};