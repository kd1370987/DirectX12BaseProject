#pragma once

namespace Engine::Raytracing
{
	// ===================================================================================
	// GPU(HLSL) 転送用データ構造
	// ※ StructuredBuffer として HLSL に送るため、16バイト(float4)アライメントを厳密に管理
	// ===================================================================================

	/// <summary>
	/// レイトレーシング空間上の1メッシュの描画パラメータ
	/// </summary>
	struct InstanceData
	{
		// --- 16 Bytes (Offset: 0) ---
		UINT materialOffset;    // Globalマテリアル配列における、このインスタンスの開始位置
		UINT vertexStart;       // 頂点バッファ内の参照開始オフセット (MegaBuffer または AnimatedBuffer 内)
		UINT indexStart;        // インデックスバッファ内の参照開始オフセット
		UINT indexCount;        // このインスタンスが持つインデックス数

		// --- 16 Bytes (Offset: 16) ---
		UINT isAnimated;        // アニメーション対象かどうかのフラグ (0: Static, 1: Animated)
		DXSM::Vector3 pad0;     // 16バイトアライメント用のパディング
	}; // Total: 32 Bytes

	/// <summary>
	/// PBRマテリアルデータ (サブメッシュごとに設定)
	/// </summary>
	struct Material
	{
		// --- 16 Bytes (Offset: 0) ---
		DXSM::Vector4       baseColor;              // xyz: BaseColor, w: Alpha

		// --- 16 Bytes (Offset: 16) ---
		DirectX::XMFLOAT3   emissive;               // 発光カラー
		float               metallic;               // 金属度

		// --- 16 Bytes (Offset: 32) ---
		float               roughness;              // 粗さ
		UINT                baseIndex;              // BaseColorテクスチャのSRVインデックス (Bindless)
		UINT                metaRoughnessIndex;     // Metallic/RoughnessテクスチャのSRVインデックス
		UINT                emissiveIndex;          // EmissiveテクスチャのSRVインデックス

		// --- 16 Bytes (Offset: 48) ---
		UINT                normalIndex;            // NormalマップテクスチャのSRVインデックス
		UINT                startIndexLocation;     // インデックスバッファ内のサブメッシュ開始位置
		DirectX::XMFLOAT2   pad0;                   // 16バイトアライメント用のパディング
	}; // Total: 64 Bytes

	// ===================================================================================
	// CPU側 レイワールド構築用データ構造
	// ===================================================================================

	/// <summary>
	/// レイワールド（TLAS）に登録するインスタンスの管理単位
	/// 静的・動的どちらのモデルもこの形式に正規化されてコミットされる
	/// </summary>
	struct Instance
	{
		// 空間情報
		DXSM::Matrix worldMat = DXSM::Matrix::Identity; // TLASに登録する際のトランスフォーム
		const BLAS* pBLAS = nullptr;                    // 参照するBLAS（動的モデルの場合は毎フレーム更新されたBLASを指す）

		// --- ジオメトリ参照情報 ---
		// HLSLのBindless配列に渡すためのハンドル
		RangeHandle<Resource::MeshVertexFloat> megaVertexHandle = {};	// 静的: StaticMegaBuffer, 動的: AnimatedBuffer
		RangeHandle<uint32_t> megaIndexHandle = {};						 // IndexBuffer (基本的に静的と共通)

		// バッファ内の論理的なオフセット
		UINT vertexOffset = 0;                          // vertexStart に相当
		UINT indexOffset = 0;                           // indexStart に相当
		UINT indexCount = 0;                            // indexCount に相当

		// 状態フラグ
		bool isAnimated = false;                        // 動的モデル判定用

		// --- 描画メタデータ ---
		std::vector<Material> submeshMaterials;         // サブメッシュごとのマテリアル情報
	};

	// １メッシュにつき一つ
	struct SkinningMeshData
	{
		// コンピュートシェーダーで書き込む変形後のバッファ
		RangeHandle<Resource::MeshVertexFloat> animatedVertexHandle = {};

		// インスタンス専用のBLAS
		Raytracing::BLAS instanceBLAS;

		// どのメッシュの参照先か
		Handle<Resource::Mesh> meshHandle;
	};

	/// <summary>
	/// レイアニメーション用構造体
	/// </summary>
	struct DynamicRaytracingData
	{
		// モデルのスキニングするメッシュすべて
		std::vector<SkinningMeshData> meshDataVec;
	};

	struct DynamicRaytracingInitRequest
	{
		// 初期化先のハンドル
		Handle<DynamicRaytracingData> dynamicInstanceHandle;
		// 初期化に必要な元モデルのハンドル
		Engine::Handle<Engine::Resource::Model> modelHandle;
	};

	struct DynamicRaytracingRequest
	{
		DXSM::Matrix worldMat;				// ワールド行列
		DXSM::Vector4 colorScale;			// 色スケール
		DXSM::Vector3 emissiveScale;		// エミッシブスケール

		Engine::Handle<DynamicRaytracingData> dynamicHandle = {};
		Engine::Handle<Resource::NodePoseMatrix> nodePoseHnandle = {};
	};
}