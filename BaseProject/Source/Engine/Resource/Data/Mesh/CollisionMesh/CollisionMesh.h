#pragma once

namespace Engine::Resource
{
	// 当たり判定用座標付きトライアングル
	struct CollisionTriangle
	{
		DirectX::XMFLOAT3 v[3];
	};

	// 一つのボックス・依存関係
	struct BVHNode
	{
		void Archive(Persistence::Archive& a_ar,int a_idx);

		bool IsLeaf() const { return (leftChild == -1); }

		DirectX::BoundingBox box = {};		// このノードを包むAABB
		int leftChild = -1;			// 左のノード（-1なら葉ノード）
		int rightChild = -1;		// 右のノード

		// 葉ノードの場合のみ、含まれるポリゴンのインデックス
		int dataStart = 0;		// スタート位置
		int dataCount = 0;		// ポリゴン数
	};


	// コリジョンメッシュ
	struct CollisionMesh
	{
		// 保存
		void Archive(Persistence::Archive& a_ar);

		// 作成
		void Create(const std::vector<DirectX::XMFLOAT3>& a_vertices,const std::vector<UINT>& a_indices);

		// 解放
		void Release();

		DirectX::BoundingBox _localAABB = {};		// メッシュ全体のローカルAABB

		std::vector<CollisionTriangle> triangleVec = {};    // 判定用ポリゴン配列
		std::vector<BVHNode> nodeVec = {};					// ノード配列
		std::vector<int> triangleIndiccesVec = {};			// 全ノードポリゴン

		// ルートノードインデックス
		int rootNodeIndex = 0;
	};
}