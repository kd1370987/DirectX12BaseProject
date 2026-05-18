#pragma once

namespace Engine::Resource
{
	// 当たり判定用座標付きトライアングル
	struct CollisionTriangle
	{
		DirectX::XMFLOAT3 v[3];
	};

	// 一つのボックス・依存関係
	struct VBHNode
	{
		DirectX::BoundingBox box = {};		// このノードを包むAABB
		int leftChild = -1;			// 左のノード（-1なら葉ノード）
		int rightChild = -1;		// 右のノード

		// 葉ノードの場合のみ、含まれるポリゴンのインデックス
		int triangleStart = 0;		// スタート位置
		int triangleCount = 0;		// ポリゴン数
	};


	// コリジョンメッシュ
	struct CollisionMesh
	{

		void Create(const std::vector<DirectX::XMFLOAT3>& a_vertices,const std::vector<UINT>& a_indices);

		DirectX::BoundingBox _localAABB = {};		// メッシュ全体のローカルAABB

		std::vector<CollisionTriangle> triangleVec = {};    // 判定用ポリゴン配列
		std::vector<VBHNode> nodeVec = {};					// ノード配列
		std::vector<int> triangleIndiccesVec = {};			// 全ノードポリゴン

		// ルートノードインデックス
		int rootNodeIndex = 0;
	};
}