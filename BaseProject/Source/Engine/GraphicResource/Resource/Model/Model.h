#pragma once

#include "ModelResource/Material/Material.h"
#include "ModelResource/Animation/Animation.h"
#include "ModelResource/Mesh/Mesh.h"
#include "ModelResource/Node/Node.h"

struct Model
{
	// マテリアル
	std::vector<Material>						materials;				// マテリアルの配列

	// アニメーション
	std::vector<std::shared_ptr<AnimationData>> spAnimations;				// データリスト

	// ノード
	std::vector<Node>							originalNodes;			// 全ノード配列
	std::vector<int>							rootNodeIndices;			// Rootノード
	std::vector<int>							boneNodeIndices;			// ボーンノード
	std::vector<int>							meshNodeIndices;			// メッシュが存在するノード
	std::vector<int>							collisionMeshNodeIndices;	// 子リジョンメッシュが存在するノード
	std::vector<int>							drawMeshNodeIndices;		// 描画するノード
};