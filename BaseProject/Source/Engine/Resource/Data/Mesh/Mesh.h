#pragma once

#include "MeshMetaData/MeshMetaData.h"
#include "RasterizationMesh/RasterizationMesh.h"
#include "RaytracingMesh/RaytracingMesh.h"
#include "CollisionMesh/CollisionMesh.h"

namespace Engine::Resource
{
	//==========================================================
	// 
	// メッシュ用クラス
	// 
	//==========================================================
	class Mesh
	{
	public:

		//=================================================
		// 作成・解放
		//=================================================
		Mesh() = default;
		~Mesh() = default;

		// メッシュ作成
		bool CreateFloat(
			const std::vector<MeshVertexFloat>& a_vertices,		// 頂点配列
			const std::vector<MeshFace>& a_face,				// 面インデックス情報配列
			const std::vector<MeshSubset>& a_subsets,			// サブセット情報配列
			bool								a_isSkinMesh	// スキンメッシュ持ちかどうか
		);
		// メッシュメタデータ作成
		void CreateMeshMetaData(
			const std::vector<MeshVertexFloat>& a_vertices,
			const std::vector<MeshSubset>& a_subsets,
			bool a_isSkinMesh
		);
		// ラスタライザ用データ作成
		void CreateRasterData(
			ID3D12Device* a_pDevice,
			const std::vector<MeshVertexFloat>& a_vertices,
			const std::vector<MeshFace>& a_face,
			DXGI_FORMAT a_indexFormat
		);
		// レイトレーシングデータ作成
		void CreateRtData(
			ID3D12Device* a_pDevice,
			D3D12::CommandList* a_pCmdList,
			const std::vector<MeshSubset>& a_subset,							// サブセット配列
			const D3D12::DynamicVertexBuffer<MeshVertexFloat>& a_vertexBuffer,	// 頂点バッファ
			DXGI_FORMAT a_vertexFarstFormat,
			const D3D12::DynamicIndexBuffer& a_indexBuffer,						// インデックスバッファ
			const std::vector<MeshVertexFloat>& a_vertices,
			const std::vector<MeshFace>& a_face
		);
		// BVHでの当たり判定構築
		void CreateCollisionMesh(
			const std::vector<DirectX::XMFLOAT3>& a_vertices, 
			const std::vector<UINT>& a_indices
		);

		//=================================================
		// アクセサ
		//=================================================
		// メタデータ
		const MeshMetaData& GetMetaData() const { return m_meshMetaData; }
		// ラスタライザデータ
		bool HasRasterData() const { return m_opRasterData.has_value(); }
		const RasterizationMesh& GetRasterData()const { return m_opRasterData.value(); }
		// レイトレデータ
		bool HasRtData() const { return m_opRtData.has_value(); }
		const RaytracingMesh& GetRtData() const { return m_opRtData.value(); }
		// 当たり判定データ
		bool HasCollisionMesh() const { return m_opCollMesh.has_value(); };				// 当たり判定を持っているかどうか
		const CollisionMesh& GetCollisionMesh()const { return m_opCollMesh.value(); }	// 当たり判定取得

	private:

		// メッシュメタデータ
		MeshMetaData m_meshMetaData;

		// 各ドメインデータ : 必要なもののみ実体化
		std::optional<RasterizationMesh>		m_opRasterData;		// ラスタライザデータ
		std::optional<RaytracingMesh>			m_opRtData;			// レイトレデータ
		std::optional<CollisionMesh>			m_opCollMesh;		// 当たり判定

	private:
		// コピー禁止
		Mesh(const Mesh& src) = delete;
		void operator=(const Mesh& src) = delete;
	};
}