#include "Node.h"

#include "../../../Utility/BinaryHelper/BinaryHelper.h"


void Engine::Resource::Node::Archive(Persistence::Archive& a_ar, int a_idx)
{
	a_ar.StringField("NodeName" + std::to_string(a_idx), name);
	a_ar.VectorField("MeshIndices" + std::to_string(a_idx), meshIndices);

	a_ar.Field("LocalTransform" + std::to_string(a_idx),localTransform);
	a_ar.Field("WorldTransform" + std::to_string(a_idx),worldTransform);
	a_ar.Field("BoneInverseWorldMatrix" + std::to_string(a_idx), boneInverseWorldMatrix);

	a_ar.Field("Parent" + std::to_string(a_idx),parent);
	a_ar.VectorField("Children" + std::to_string(a_idx), children);
	a_ar.Field("BoneIndex" + std::to_string(a_idx), boneIndex);
	a_ar.Field("IsSkinMesh" + std::to_string(a_idx), isSkinMesh);
}
