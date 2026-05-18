#include "MeshMetaData.h"

namespace Engine::Resource
{
	void MeshMetaData::Create(
		const std::vector<MeshVertexFloat>& a_vertices,
		const std::vector<MeshSubset>& a_subsets, 
		bool a_isSkinMesh
	)
	{
		// データ挿入
		subsets.clear();
		subsets = a_subsets;			// サブセット配列
		isSkinMesh = a_isSkinMesh;		// スキンメッシュを持ってるかどうか

		// 頂点の座標のみを集める
		std::vector<DirectX::XMFLOAT3> _posVec = {};
		_posVec.resize(a_vertices.size());
		for (size_t _i = 0; _i < a_vertices.size(); ++_i)
		{
			_posVec[_i] = a_vertices[_i].pos;
		}

		// 頂点情報から境界データ作成
		DirectX::BoundingBox::CreateFromPoints(
			aabb,_posVec.size(),_posVec.data(), sizeof(DirectX::XMFLOAT3)
		);
		DirectX::BoundingSphere::CreateFromPoints(
			bSphere,_posVec.size(),_posVec.data(),sizeof(DirectX::XMFLOAT3)
		);
	}
}