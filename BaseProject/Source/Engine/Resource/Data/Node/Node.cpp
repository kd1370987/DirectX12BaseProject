#include "Node.h"

#include "../../../Utility/BinaryHelper/BinaryHelper.h"


void Engine::Resource::Node::Archive(Persistence::Archive& a_ar)
{
	a_ar.StringField("NodeName", name);
	a_ar.VectorField("MeshIndices", meshIndices);

	a_ar.Field("LocalTransform",localTransform);
	a_ar.Field("WorldTransform",worldTransform);
	a_ar.Field("BoneInverseWorldMatrix", boneInverseWorldMatrix);

	a_ar.Field("Parent",parent);
	a_ar.VectorField("Children", children);
	a_ar.Field("BoneIndex", boneIndex);
	a_ar.Field("IsSkinMesh", isSkinMesh);
}
