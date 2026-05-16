#pragma once
namespace Engine::Resource
{
	//==========================================================
	// メッシュ用 面情報
	//==========================================================
	struct MeshFace
	{
		UINT idx[3];			// 三角形を構成する頂点のIndex
	};

	//==========================================================
	// メッシュ用 サブセット情報
	//==========================================================
	struct MeshSubset
	{
		UINT materialNumber = 0;		// マテリアルナンバー
		UINT faceStart = 0;				// 面Index : このマテリアルで使用されている最初の面のIndex
		UINT faceCount = 0;				// 面数    : faceStartから、何枚の面が使用されているかの数
	};

	//==========================================================
	// 共通メタデータ
	// どのメッシュでも共通して使う軽いデータ
	//==========================================================
	struct MeshMetaData
	{
		// 作成
		void Create(
			const std::vector<MeshVertexFloat>& a_vertices, 
			const std::vector<MeshSubset>& a_subsets,
			bool a_isSkinMesh
		);

		std::vector<MeshSubset> subsets;
		DirectX::BoundingBox aabb;
		DirectX::BoundingSphere bSphere;
		bool isSkinMesh = false;
	};
}