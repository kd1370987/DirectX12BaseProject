#include "CollisionMesh.h"

//===============================================================================================
// 
// グリッドサイズ計算
// 
//===============================================================================================
int CalcGridSize(
	float a_length,
	float a_cellSize
)
{
	// 四捨五入
	int _res = static_cast<int>(
		std::ceil((a_length / 2.0f))
		);

	// 0にはならないように
	return std::max(1, _res);
}

//===============================================================================================
// 
// バウンディングボックス分割
// 
//===============================================================================================



Engine::Collision::Mesh Engine::Collision::CreateMesh(
	const std::vector<DirectX::XMFLOAT3>& a_positionVec,
	const std::vector<DirectX::XMFLOAT3>& a_faceVec
)
{
	Engine::Collision::Mesh _mesh = {};

	// ローカルボックス作成
	DirectX::BoundingBox::CreateFromPoints(
		_mesh.localAABB,
		a_positionVec.size(),
		&a_positionVec[0],
		sizeof(DirectX::XMFLOAT3)
	);

	// トライアングルリスト作成
	_mesh.triangleVec.resize(a_faceVec.size());
	for (int _i = 0; _i < _mesh.triangleVec.size(); ++_i)
	{
		Engine::Collision::Triangle& _tri = _mesh.triangleVec[_i];

		// 頂点情報格納
		_tri.v[0] = a_positionVec[a_faceVec[_i].x];
		_tri.v[1] = a_positionVec[a_faceVec[_i].y];
		_tri.v[2] = a_positionVec[a_faceVec[_i].z];

		// ポリゴンボックス作成
		DirectX::BoundingBox::CreateFromPoints(
			_tri.AABB,
			3,
			&_tri.v[0],
			sizeof(DirectX::XMFLOAT3)
		);
	}

	// グリッド作成
	_mesh.grid = CreateGrid(
		_mesh.localAABB,
		_mesh.triangleVec
	);

	return _mesh;
}



Engine::Collision::Grid Engine::Collision::CreateGrid(
	const DirectX::BoundingBox& a_localAABB,
	const std::vector<Triangle>& a_triangleVec
)
{
	Engine::Collision::Grid _grid = {};

	// 基準となるサイズ
	float _gridSize = 2.0f;

	// 軸ごとの分割数を決めていく
	_grid.countX = CalcGridSize(a_localAABB.Extents.x * 2, _gridSize);
	_grid.countY = CalcGridSize(a_localAABB.Extents.y * 2, _gridSize);
	_grid.countZ = CalcGridSize(a_localAABB.Extents.z * 2, _gridSize);

	// セルの総数を求めて配列を作成
	int _gridNum = _grid.countX * _grid.countY * _grid.countZ;
	_grid.cellVec.resize(_gridNum);

	// バウンディングボックス内の最小と最大の頂点を求める（対角線上）
	DirectX::XMFLOAT3 _min = {};
	DirectX::XMFLOAT3 _max = {};

	// 左下頂点を求める
	_min.x = a_localAABB.Center.x - a_localAABB.Extents.x;
	_min.y = a_localAABB.Center.y - a_localAABB.Extents.y;
	_min.z = a_localAABB.Center.z - a_localAABB.Extents.z;

	// 右上頂点を求める
	_max.x = a_localAABB.Center.x + a_localAABB.Extents.x;
	_max.y = a_localAABB.Center.y + a_localAABB.Extents.y;
	_max.z = a_localAABB.Center.z + a_localAABB.Extents.z;

	// セルサイズの決定
	float _cellSizeX = (_max.x - _min.x) / _grid.countX;
	float _cellSizeY = (_max.y - _min.y) / _grid.countY;
	float _cellSizeZ = (_max.z - _min.z) / _grid.countZ;

	// セルごとのバウンディングボックスの作成
	for (int _x = 0; _x < _grid.countX; ++_x)
	{
		for (int _y = 0; _y < _grid.countY; ++_y)
		{
			for (int _z = 0; _z < _grid.countZ; ++_z)
			{
				// 配列上の位置を求める
				size_t _index = _x + (_y * _grid.countX) + (_z * _grid.countX * _grid.countY);

				// バウンディングボックス内の最小と最大の頂点を求める（対角線上）
				DirectX::XMFLOAT3 _cellMin = {};
				DirectX::XMFLOAT3 _cellMax = {};

				// 左下頂点を求める
				_cellMin.x = _min.x + _x * _cellSizeX;
				_cellMin.y = _min.y + _y * _cellSizeY;
				_cellMin.z = _min.z + _z * _cellSizeZ;

				// 右上頂点を求める
				_cellMax.x = _cellMin.x + _cellSizeX;
				_cellMax.y = _cellMin.y + _cellSizeY;
				_cellMax.z = _cellMin.z + _cellSizeZ;

				// バウンディングボックス作成
				DirectX::XMFLOAT3 _center = {};
				DirectX::XMFLOAT3 _extents = {};

				_center.x = (_cellMin.x + _cellMax.x) * 0.5f;
				_center.y = (_cellMin.y + _cellMax.y) * 0.5f;
				_center.z = (_cellMin.z + _cellMax.z) * 0.5f;

				_extents.x = (_cellMax.x - _cellMin.x) * 0.5f;
				_extents.y = (_cellMax.y - _cellMin.y) * 0.5f;
				_extents.z = (_cellMax.z - _cellMin.z) * 0.5f;

				_grid.cellVec[_index].box = DirectX::BoundingBox(_center,_extents);

				// セル内のポリゴンを求める
				for (size_t _i = 0; _i < a_triangleVec.size(); ++_i)
				{
					if (_grid.cellVec[_index].box.Intersects(a_triangleVec[_i].AABB))
					{
						_grid.cellVec[_index].triangleVec.push_back(_i);
					}
				}
			}
		}
	}

	return _grid;
}
