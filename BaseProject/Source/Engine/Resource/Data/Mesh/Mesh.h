#pragma once

namespace Engine
{
	namespace Resource
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

			void CreateCollision();

			/// <summary>
			/// メッシュの解放と初期化
			/// </summary>
			void Release();

			// BLAS作成
			void CreateBLAS();

			//=================================================
			// アクセサ
			//=================================================
			const D3D12::VertexBuffer<MeshVertexFloat>& GetVertexBuffer()		const { return m_vertexBuffer; }		// 頂点バッファ取得
			const IndexBuffer& GetIndexBuffer()			const { return m_indexBuffer; }			// インデックスバッファ取得

			const std::vector<MeshSubset>& GetSubsets() const { return m_subsets; }				// サブセット情報取得
			const std::vector<DirectX::XMFLOAT3>& GetPositions() const { return m_positions; }	// 座標配列取得
			const std::vector<MeshFace>& GetFaces()		const { return m_faces; }				// 面情報配列取得

			const DirectX::BoundingBox& GetAABB()		const { return m_aabb; }				// 軸平行境界ボックス取得
			const DirectX::BoundingSphere& GetBSphere()	const { return m_bSphere; }				// 境界球取得

			bool IsSkinMesh()							const { return m_isSkinMesh; }			// スキンメッシュ持ちかどうか

			bool HasCollision() const { return m_opCollisionMesh.has_value(); };				// 当たり判定を持っているかどうか
			const Engine::Collision::Mesh& GetCollision()const { return *m_opCollisionMesh; }	// 当たり判定取得

			Engine::Raytracing::BLAS* GetBLAS() { return &m_BLAS; }

			const std::vector<MeshVertexFloat>& GetVertexData() const  { return m_vertexData; }
			const std::vector<UINT>& GetIndexData()const { return m_indexData; }

			const Engine::D3D12::StructuredBuffer<RTVertex>& GetSVertexBuff() { return m_sVertexBuffer; };
			const Engine::D3D12::StructuredBuffer<UINT>& GetSIndexBuff() { return m_sIndexBuffer; }

		private:

			// バッファ
			D3D12::VertexBuffer<MeshVertexFloat> m_vertexBuffer;		// 頂点バッファ
			IndexBuffer							 m_indexBuffer;		// インデックスバッファ

			// サブセット情報
			std::vector<MeshSubset>			m_subsets;

			// 境界データ
			DirectX::BoundingBox			m_aabb;				// 軸平行境界ボックス
			DirectX::BoundingSphere			m_bSphere;			// 境界球

			// 座標のみの配列情報
			std::vector<DirectX::XMFLOAT3>	m_positions;

			// 面情報のみの配列
			std::vector<MeshFace>			m_faces;

			// スキンメッシュかどうか
			bool							m_isSkinMesh = false;

			// 当たり判定
			std::optional<Engine::Collision::Mesh> m_opCollisionMesh;

			// レイトレ用データ
			Engine::Raytracing::BLAS m_BLAS;

			std::vector<MeshVertexFloat> m_vertexData = {};
			std::vector<UINT> m_indexData = {};

			Engine::D3D12::StructuredBuffer<RTVertex> m_sVertexBuffer;
			Engine::D3D12::StructuredBuffer<UINT> m_sIndexBuffer;
		private:
			// コピー禁止
			Mesh(const Mesh& src) = delete;
			void operator=(const Mesh& src) = delete;
		};
	}
}