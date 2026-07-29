#pragma once

#include "MeshMetaData/MeshMetaData.h"
#include "RasterizationMesh/RasterizationMesh.h"
#include "RaytracingMesh/RaytracingMesh.h"
#include "CollisionMesh/CollisionMesh.h"
#include "MeshShaderData/MeshShaderData.h"

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
		NON_COPYABLE_MOVABLE(Mesh);

		/// <summary>
		/// メッシュ作成
		///
		/// GPUへの転送・BLAS構築はコンテキストのコマンドリストへ積むだけで、
		/// キューへの実行は行わない。実行はバッチを開いた側(モデルのインポート処理など)の責任。
		/// </summary>
		/// <param name="a_ctx">ビルドコンテキスト</param>
		bool CreateFloat(
			const ResourceBuildContext& a_ctx,					// ビルドコンテキスト
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
			D3D12::Device* a_pDevice,
			const std::vector<MeshVertexFloat>& a_vertices,
			const std::vector<MeshFace>& a_face,
			DXGI_FORMAT a_indexFormat
		);
		// レイトレーシングデータ作成 : コンテキストのコンピュートリストへBLAS構築を積む
		void CreateRtData(
			const ResourceBuildContext& a_ctx,
			const std::vector<MeshSubset>& a_subset								// サブセット配列
		);
		// BVHでの当たり判定構築
		void CreateCollisionMesh(
			const std::vector<DirectX::XMFLOAT3>& a_vertices,
			const std::vector<UINT>& a_indices
		);
		// メッシュシェーダー用データの作成 : メッシュレット生成(CPU)と転送コマンドの記録
		void CreateMeshShaderData(
			const ResourceBuildContext& a_ctx,
			const std::vector<MeshVertexFloat>& a_vertices,
			const std::vector<MeshFace>& a_face
		);

		// 解放
		void Release();

		//=================================================
		// 保存
		//=================================================
		void Save(const std::string& a_fileDir, const std::string& a_name);
		void Load(const ResourceBuildContext& a_ctx, const std::string& a_fileDir, const std::string& a_name);
		void Load(const ResourceBuildContext& a_ctx, const std::string& a_filePath);

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
		// メッシュレットデータ
		bool HasMeshShaderData() const { return m_opMeshShaderData.has_value(); }
		const MeshShaderData& GetMeshShaderData() const { return m_opMeshShaderData.value(); }
		// 当たり判定データ
		bool HasCollisionMesh() const { return m_opCollMesh.has_value(); };				// 当たり判定を持っているかどうか
		const CollisionMesh& GetCollisionMesh()const { return m_opCollMesh.value(); }	// 当たり判定取得

		// データ取得
		const std::vector<MeshVertexFloat>& GetVertexVec() const { return m_vertices; }
		std::vector<MeshVertexFloat>& RefVertexVec(){ return m_vertices; }
		const std::vector<MeshFace>& GetFaceVec() const { return m_face; }	// 面インデックス(描画メッシュの三角形)

	private:

		// メッシュメタデータ
		MeshMetaData m_meshMetaData;

		// 各ドメインデータ : 必要なもののみ実体化
		std::optional<RasterizationMesh>		m_opRasterData;		// ラスタライザデータ
		std::optional<RaytracingMesh>			m_opRtData;			// レイトレデータ
		std::optional<CollisionMesh>			m_opCollMesh;		// 当たり判定
		std::optional<MeshShaderData>			m_opMeshShaderData;	// メッシュシェーダーデータ

		// セーブ用データ
		std::vector<MeshVertexFloat> m_vertices;	// 頂点配列
		std::vector<MeshFace> m_face;				// 面インデックス情報配列
		std::vector<MeshSubset> m_subsets;			// サブセット情報配列
		bool m_isSkinMesh;							// スキンメッシュ持ちかどうか

	};
}