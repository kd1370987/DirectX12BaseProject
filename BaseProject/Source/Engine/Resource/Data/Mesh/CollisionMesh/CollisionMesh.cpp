#include "CollisionMesh.h"

namespace Engine::Resource
{
	namespace
	{
		//----------------------------------------------------------------------------------
		// 以前の保存形式に入っている BVH ノード。
		//
		// 自作の当たり判定を Jolt へ移したので BVH はもう作らないが、.mesh の保存形式は
		// バイナリ固定で、フィールドを抜くと既存のファイルが読めなくなる。
		// なので形式はそのままにして、読むときはここへ読み飛ばして捨て、
		// 書くときは 0 個として書く。並び(キー名と順番)は変えないこと
		//----------------------------------------------------------------------------------
		struct LegacyBVHNode
		{
			DirectX::BoundingBox box = {};
			int leftChild = -1;
			int rightChild = -1;
			int dataStart = 0;
			int dataCount = 0;

			void Archive(Persistence::Archive& a_ar, int a_idx)
			{
				a_ar.Field("BVHNode_BoxCenter" + std::to_string(a_idx), box.Center);
				a_ar.Field("BVHNode_BoxExtents" + std::to_string(a_idx), box.Extents);

				a_ar.Field("BVHNode_LeftChild" + std::to_string(a_idx), leftChild);
				a_ar.Field("BVHNode_RightChild" + std::to_string(a_idx), rightChild);

				a_ar.Field("BVHNode_DataStart" + std::to_string(a_idx), dataStart);
				a_ar.Field("BVHNode_DataCount" + std::to_string(a_idx), dataCount);
			}
		};
	}

	void CollisionMesh::Archive(Persistence::Archive& a_ar)
	{
		a_ar.Field("BoxCenter", _localAABB.Center);
		a_ar.Field("BoxExtents", _localAABB.Extents);

		// 三角形配列のサイズを保存・復元してリサイズ
		size_t _triSize = triangleVec.size();
		a_ar.Field("TriangleCount", _triSize); // Load時はファイルから個数が _triSize に上書きされる
		triangleVec.resize(_triSize);          // 適切なサイズにリサイズ！

		int _i = 0;
		for (auto& _triangle : triangleVec)
		{
			// キー名を "_v0" などに変更して誤解を防ぐ
			a_ar.Field("triangle_" + std::to_string(_i) + "_v0", _triangle.v[0]);
			a_ar.Field("triangle_" + std::to_string(_i) + "_v1", _triangle.v[1]);
			a_ar.Field("triangle_" + std::to_string(_i) + "_v2", _triangle.v[2]);
			_i++;
		}

		//----------------------------------------------------------------------------------
		// 以前の BVH の場所。保存は 0 個、読み込みは読み飛ばして捨てる(LegacyBVHNode を参照)
		//----------------------------------------------------------------------------------
		size_t _nodeSize = 0;
		a_ar.Field("NodeCount", _nodeSize);

		LegacyBVHNode _discardNode = {};
		for (size_t _n = 0; _n < _nodeSize; ++_n)
		{
			_discardNode.Archive(a_ar, static_cast<int>(_n));
		}

		std::vector<int> _discardIndices = {};
		a_ar.VectorField("TrglIndicces", _discardIndices);

		int _discardRoot = 0;
		a_ar.Field("RootNodeIndex", _discardRoot);
	}

	void CollisionMesh::Create(
		const std::vector<Math::Vector3>& a_vertices,
		const std::vector<UINT>& a_indices
	)
	{
		UINT _triangleCount = static_cast<UINT>(a_indices.size() / 3);
		if (_triangleCount == 0) return;

		// 生の三角形配列を構築
		triangleVec.resize(_triangleCount);
		for (UINT _i = 0; _i < _triangleCount; ++_i)
		{
			triangleVec[_i].v[0] = a_vertices[a_indices[_i * 3 + 0]];
			triangleVec[_i].v[1] = a_vertices[a_indices[_i * 3 + 1]];
			triangleVec[_i].v[2] = a_vertices[a_indices[_i * 3 + 2]];
		}

		// メッシュ全体のローカルAABB(三角形の全頂点を包む)。
		// CollisionTriangle は Vector3 を3つ並べただけなので、頂点の並びとしてそのまま舐める。
		// Math::Vector3 は XMFLOAT3 とバイナリ配置が同じ(static_assert 済み)なので、詰め替えずに渡す
		static_assert(sizeof(CollisionTriangle) == sizeof(Math::Vector3) * 3, "CollisionTriangle に詰め物が入っている");
		DirectX::BoundingBox::CreateFromPoints(
			_localAABB,
			triangleVec.size() * 3,
			reinterpret_cast<const DirectX::XMFLOAT3*>(triangleVec.data()),
			sizeof(Math::Vector3));
	}

	void CollisionMesh::Release()
	{
		triangleVec.clear();
	}
}
