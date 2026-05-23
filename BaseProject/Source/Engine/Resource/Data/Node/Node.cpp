#include "Node.h"

#include "../../../Utility/BinaryHelper/BinaryHelper.h"

void Engine::Resource::Node::Save(std::ofstream& a_ofs)
{
	// ノード名
	BinaryHelper::WriteString(a_ofs, name);

	// メッシュインデックス
	BinaryHelper::WriteVector(a_ofs, meshIndices);

	// 各行列
	BinaryHelper::Write(a_ofs,localTransform);
	BinaryHelper::Write(a_ofs, worldTransform);
	BinaryHelper::Write(a_ofs, boneInverseWorldMatrix);

	// 依存関係
	BinaryHelper::Write(a_ofs, parent);
	BinaryHelper::Write(a_ofs, children);
	BinaryHelper::Write(a_ofs, boneIndex);
	BinaryHelper::Write(a_ofs, isSkinMesh);
}
