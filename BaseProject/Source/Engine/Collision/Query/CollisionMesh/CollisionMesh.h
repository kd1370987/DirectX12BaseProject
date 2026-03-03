#pragma once

namespace Engine
{
	namespace Collision
	{
		// 当たり判定用三角形ポリゴン
		struct Triangle
		{
			DirectX::XMFLOAT3		v[3];			// 三角形ポリゴン
			DirectX::BoundingBox	AABB = {};			// 三角形ごとの当たり判定ボックス
		};

		// グリッドごとの情報
		struct Cell
		{
			DirectX::BoundingBox	box = {};
			std::vector<int>		triangleVec = {};
		};

		// 当たり判定用メッシュの分割グリッド
		struct Grid
		{
			// 基準モデルのサイズに左右されるグリッドの作成サイズ
			int					countX = 0;
			int					countY = 0;
			int					countZ = 0;

			// AABBを当分割した配列
			std::vector<Cell>	cellVec = {};
		};

		// 当たり判定用メッシュ
		struct Mesh
		{
			DirectX::BoundingBox	localAABB = {};		// メッシュ全体のローカルボックス
			Grid					grid = {};			// ローカルボックスを等分割したもの
			std::vector<Triangle>	triangleVec = {};	// メッシュの当たり判定用ポリゴンリスト
		};

		// 当たり判定メッシュ生成関数
		Engine::Collision::Mesh CreateMesh(
			const std::vector<DirectX::XMFLOAT3>& a_positionVec,
			const std::vector<DirectX::XMFLOAT3>& a_faceVec
		);

		// グリッド生成
		Engine::Collision::Grid CreateGrid(
			const DirectX::BoundingBox& a_localAABB,
			const std::vector<Triangle>& a_triangleVec
		);
	}
}