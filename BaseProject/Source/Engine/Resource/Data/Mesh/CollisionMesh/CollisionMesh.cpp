#include "CollisionMesh.h"
namespace Engine::Resource
{
	struct BuildTriangle
	{
		int originalIndex = 0;			// 元のトライアングルでのインデックス
		DirectX::XMFLOAT3 centroid;		// 三角形の中心点
	};

	// 再帰的にBVHを子駆逐する内部関数
	int BuildBVHInternal(
		CollisionMesh& a_outMesh,
		std::vector<BuildTriangle>& a_buildTriangles,
		int a_start,
		int a_count,
		const int a_maxTrianglesPerLeaf = 4		//一つの葉ノードに入れる最大ポリゴン数
	)
	{
		// 新しいノードを作成して配列に追加
		BVHNode _node;
		int _nodeIndex = static_cast<int>(a_outMesh.nodeVec.size());
		a_outMesh.nodeVec.push_back(_node);

		// このノードに含まれる全三角形を含むAABBを計算
		// 全頂点を集めて１つのAABBを作成
		std::vector<DirectX::XMFLOAT3> _points;
		_points.reserve(a_count * 3);
		for (int _i = 0; _i < a_count; ++_i)
		{
			int _triIdx = a_buildTriangles[a_start + _i].originalIndex;
			const auto& _tri = a_outMesh.triangleVec[_triIdx];
			_points.push_back(_tri.v[0]);
			_points.push_back(_tri.v[1]);
			_points.push_back(_tri.v[2]);
		}
		DirectX::BoundingBox::CreateFromPoints(
			a_outMesh.nodeVec[_nodeIndex].box, _points.size(), _points.data(), sizeof(DirectX::XMFLOAT3));

		// 終了条件
		// ポリゴン数が閾値以下なら葉ノードにする
		if (a_count <= a_maxTrianglesPerLeaf)
		{
			a_outMesh.nodeVec[_nodeIndex].leftChild = -1;
			a_outMesh.nodeVec[_nodeIndex].rightChild = -1;
			a_outMesh.nodeVec[_nodeIndex].dataStart = static_cast<int>(a_outMesh.triangleIndiccesVec.size());
			a_outMesh.nodeVec[_nodeIndex].dataCount = a_count;

			// 葉ノードが参照するポリゴンインデックスを確定させる
			for (int _i = 0; _i < a_count; ++_i)
			{
				a_outMesh.triangleIndiccesVec.push_back(a_buildTriangles[a_start + _i].originalIndex);
			}

			return _nodeIndex;
		}

		// 枝ノードの場合 
		// 一番広がっている軸(x,y,z)を見つける
		const auto& _extents = a_outMesh.nodeVec[_nodeIndex].box.Extents;
		int _axis = 0;	 // 0 : x , 1 : y , 2 : z
		if (_extents.y > _extents.x && _extents.y > _extents.z) _axis = 1;
		if (_extents.z > _extents.x && _extents.z > _extents.y) _axis = 2;

		// 選んだ軸の座標で三角形をソートする
		std::sort(a_buildTriangles.begin() + a_start, a_buildTriangles.begin() + a_start + a_count,
			[_axis](const BuildTriangle& a, const BuildTriangle& b)
			{
				if (_axis == 0) return a.centroid.x < b.centroid.x;
				if (_axis == 1) return a.centroid.y < b.centroid.y;
				return a.centroid.z < b.centroid.z;
			}
		);

		// 中央値で分割して再帰ビルド
		int _mid = a_count / 2;

		// 左側の子をビルド
		int _leftChildIndex = BuildBVHInternal(a_outMesh,a_buildTriangles,a_start,_mid,a_maxTrianglesPerLeaf);
		// 右側の子をビルド
		int _rightChildIndex = BuildBVHInternal(a_outMesh,a_buildTriangles,a_start + _mid,a_count - _mid,a_maxTrianglesPerLeaf);

		// 親ノードに子供のインデックスを設定
		a_outMesh.nodeVec[_nodeIndex].leftChild = _leftChildIndex;
		a_outMesh.nodeVec[_nodeIndex].rightChild = _rightChildIndex;

		return _nodeIndex;
	}

	void CollisionMesh::Archive(Persistence::Archive& a_ar)
	{
		a_ar.Field("BoxCenter",_localAABB.Center);
		a_ar.Field("BoxExtents",_localAABB.Extents);
		int _i = 0;
		for (auto& _triangle : triangleVec)
		{
			a_ar.Field("triangle_" + std::to_string(_i) + "x", _triangle.v[0]);
			a_ar.Field("triangle_" + std::to_string(_i) + "y", _triangle.v[1]);
			a_ar.Field("triangle_" + std::to_string(_i) + "z", _triangle.v[2]);
			_i++;
		}
		_i = 0;
		for (auto& _node : nodeVec)
		{
			_node.Archive(a_ar,_i);
			_i++;
		}
		a_ar.VectorField("TrglIndicces",triangleIndiccesVec);
		a_ar.Field("RootNodeIndex",rootNodeIndex);
	}

	void Engine::Resource::CollisionMesh::Create(
		const std::vector<DirectX::XMFLOAT3>& a_vertices, 
		const std::vector<UINT>& a_indices
	)
	{
		UINT _triangleCount = static_cast<UINT>(a_indices.size() / 3);
		if (_triangleCount == 0) return;

		// 生の三角形配列を構築
		triangleVec.resize(_triangleCount);
		std::vector<BuildTriangle> _buildTriangles(_triangleCount);

		// ベクターの自動拡張による無駄を防ぐため、ある程度のリザーブをしておく
		nodeVec.reserve(_triangleCount * 2);
		triangleIndiccesVec.reserve(_triangleCount);

		for (UINT _i = 0; _i < _triangleCount; ++_i)
		{
			UINT _idx0 = a_indices[_i * 3 + 0];
			UINT _idx1 = a_indices[_i * 3 + 1];
			UINT _idx2 = a_indices[_i * 3 + 2];

			triangleVec[_i].v[0] = a_vertices[_idx0];
			triangleVec[_i].v[1] = a_vertices[_idx1];
			triangleVec[_i].v[2] = a_vertices[_idx2];

			// ソート用の中央値
			_buildTriangles[_i].originalIndex = _i;
			_buildTriangles[_i].centroid.x = 
				(triangleVec[_i].v[0].x + triangleVec[_i].v[1].x + triangleVec[_i].v[2].x) / 3.0f;
			_buildTriangles[_i].centroid.y =
				(triangleVec[_i].v[0].y + triangleVec[_i].v[1].y + triangleVec[_i].v[2].y) / 3.0f;
			_buildTriangles[_i].centroid.z =
				(triangleVec[_i].v[0].z + triangleVec[_i].v[1].z + triangleVec[_i].v[2].z) / 3.0f;
		}

		// BVHのビルドを開始
		rootNodeIndex = BuildBVHInternal(*this, _buildTriangles, 0, _triangleCount);

		// メッシュ全体のローカルAABBは、ルートノードのAABBと同じになる
		if (!nodeVec.empty())
		{
			_localAABB = nodeVec[rootNodeIndex].box;
		}
	}

	void BVHNode::Archive(Persistence::Archive& a_ar,int a_idx)
	{
		a_ar.Field("BVHNode_BoxCenter" + std::to_string(a_idx),box.Center);
		a_ar.Field("BVHNode_BoxExtents" + std::to_string(a_idx),box.Extents);

		a_ar.Field("BVHNode_LeftChild" + std::to_string(a_idx),leftChild);
		a_ar.Field("BVHNode_RightChild" + std::to_string(a_idx),rightChild);

		a_ar.Field("BVHNode_DataStart" + std::to_string(a_idx),dataStart);
		a_ar.Field("BVHNode_DataCount" + std::to_string(a_idx),dataCount);
	}

}