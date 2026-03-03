#include "Mesh.h"

bool Engine::Resource::Mesh::Create(
	const std::vector<MeshVertex8bit>& a_vertices,
	const std::vector<MeshFace>& a_face,
	const std::vector<MeshSubset>& a_subsets,
	bool a_isSkinMesh
)
{
	//------------------------------
	// サブセット情報
	//------------------------------
	m_subsets = a_subsets;

	//------------------------------
	// 頂点情報があるのなら
	//------------------------------
	if (a_vertices.size() > 0)
	{
		//------------------------------
		// 頂点バッファ作成
		//------------------------------
		if (!m_vertexBuffer.Create(
			(UINT)a_vertices.size(),
			sizeof(MeshVertex8bit),
			a_vertices.data()
		))
		{
			return false;
		}

		//------------------------------
		// 座標のみの配列
		//------------------------------
		m_positions.resize(a_vertices.size());			// サイズ確保
		for (size_t _i = 0; _i < a_vertices.size(); ++_i)
		{
			m_positions[_i] = a_vertices[_i].pos;
		}

		//------------------------------
		// 境界データ作成
		//------------------------------
		DirectX::BoundingBox::CreateFromPoints(					// AA境界データ作成
			m_aabb, m_positions.size(), &m_positions[0], sizeof(DirectX::XMFLOAT3));
		DirectX::BoundingSphere::CreateFromPoints(				// 境界球データ作成
			m_bSphere, m_positions.size(), &m_positions[0], sizeof(DirectX::XMFLOAT3));
	}

	//------------------------------
	// インデックス情報があるのなら
	//------------------------------
	if (a_face.size() > 0)
	{
		m_faces = a_face;		// 面情報コピー

		//------------------------------
		// インデックスバッファ作成
		//------------------------------
		std::vector<UINT> _indices;		// インデックス配列作成
		for (auto& _f : a_face)
		{
			_indices.push_back(_f.idx[0]);
			_indices.push_back(_f.idx[1]);
			_indices.push_back(_f.idx[2]);
		}

		if (!m_indexBuffer.Create(
			(UINT)_indices.size(),
			sizeof(UINT),
			_indices.data()
		))
		{
			return false;
		}
	}

	//------------------------------
	// スキンメッシュ持ちかどうか
	//------------------------------
	m_isSkinMesh = a_isSkinMesh;

	return true;
}

bool Engine::Resource::Mesh::CreateFloat(
	const std::vector<MeshVertexFloat>& a_vertices,
	const std::vector<MeshFace>& a_face,
	const std::vector<MeshSubset>& a_subsets,
	bool a_isSkinMesh
)
{
	m_subsets.clear();
	m_positions.clear();
	m_faces.clear();

	//------------------------------
	// サブセット情報
	//------------------------------
	m_subsets = a_subsets;

	//------------------------------
	// 頂点情報があるのなら
	//------------------------------
	if (a_vertices.size() > 0)
	{
		//------------------------------
		// 頂点バッファ作成
		//------------------------------
		if (!m_vertexBuffer.Create(
			(UINT)a_vertices.size(),
			sizeof(MeshVertexFloat),
			a_vertices.data()
		))
		{
			assert(0 && "頂点バッファの生成に失敗");
			return false;
		}

		//------------------------------
		// 座標のみの配列
		//------------------------------
		m_positions.resize(a_vertices.size());			// サイズ確保
		for (size_t _i = 0; _i < a_vertices.size(); ++_i)
		{
			m_positions[_i] = a_vertices[_i].pos;
		}

		//------------------------------
		// 境界データ作成
		//------------------------------
		DirectX::BoundingBox::CreateFromPoints(					// AA境界データ作成
			m_aabb, m_positions.size(), &m_positions[0], sizeof(DirectX::XMFLOAT3));
		DirectX::BoundingSphere::CreateFromPoints(				// 境界球データ作成
			m_bSphere, m_positions.size(), &m_positions[0], sizeof(DirectX::XMFLOAT3));
	}

	//------------------------------
	// インデックス情報があるのなら
	//------------------------------
	if (a_face.size() > 0)
	{
		m_faces = a_face;		// 面情報コピー

		//------------------------------
		// インデックスバッファ作成
		//------------------------------
		std::vector<UINT> _indices;		// インデックス配列作成
		//_indices.resize(a_face.size() * 3);		// サイズ確保
		for (auto& _f : a_face)
		{
			_indices.push_back(_f.idx[0]);
			_indices.push_back(_f.idx[1]);
			_indices.push_back(_f.idx[2]);
		}

		if (!m_indexBuffer.Create(
			(UINT)_indices.size(),
			sizeof(UINT),
			_indices.data()
		))
		{
			assert(0 && "インデックスバッファの生成に失敗");
			return false;
		}
	}

	//------------------------------
	// スキンメッシュ持ちかどうか
	//------------------------------
	m_isSkinMesh = a_isSkinMesh;

	return true;
}

void Engine::Resource::Mesh::CreateCollision()
{
	std::vector<DirectX::XMFLOAT3> _vec;
	_vec.resize(m_faces.size());
	for(int _i = 0; _i < m_faces.size(); ++_i)
	{
		_vec[_i].x = m_faces[_i].idx[0];
		_vec[_i].y = m_faces[_i].idx[1];
		_vec[_i].z = m_faces[_i].idx[2];
	}
	
	// 生成
	m_opCollisionMesh = Engine::Collision::CreateMesh(
		m_positions,
		_vec
	);
}

void Engine::Resource::Mesh::Release()
{
	m_subsets.clear();
	m_positions.clear();
	m_faces.clear();
}

