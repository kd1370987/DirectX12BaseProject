#pragma once

namespace Engine::Resource
{
	// 当たり判定用座標付きトライアングル
	struct CollisionTriangle
	{
		Math::Vector3 v[3];
	};

	//======================================================================================
	// コリジョンメッシュ
	//
	// 判定用ノード(COL)の三角形をメッシュローカルのまま持つ。
	// 物理空間(Physics::PhysicsWorld)がこれを Jolt のメッシュ形状にする。
	//
	// 以前は自作の当たり判定のために三角形の BVH も作って保存していた。
	// 今は作らないが、保存形式(.mesh はバイナリ固定)はそのままにしてあるので、
	// 古いファイルに残っている BVH は読み込み時に読み飛ばして捨てる(Archive)
	//======================================================================================
	struct CollisionMesh
	{
		// 保存
		void Archive(Persistence::Archive& a_ar);

		// 作成
		void Create(const std::vector<Math::Vector3>& a_vertices,const std::vector<UINT>& a_indices);

		// 解放
		void Release();

		DirectX::BoundingBox _localAABB = {};		// メッシュ全体のローカルAABB

		std::vector<CollisionTriangle> triangleVec = {};    // 判定用ポリゴン配列
	};
}
