#pragma once

namespace Engine::Raytracing
{
	// ---- 構造体バッファ用データ ----
	// インスタンスごとのデータ
	struct InstanceData
	{
		UINT vertexSRVIndex;
		UINT indexSRVIndex;

		UINT materialOffset;	// マテリアル配列での位置
		uint32_t pad;
	};
	// マテリアルSRVに対しての個別設定データ
	// サブメッシュごとに設定
	struct Material
	{
		DXSM::Vector4		baseColor = { 1,1,1,1 };
		DirectX::XMFLOAT3	emissive = { 1.0f,1.0f,1.0f };
		float				metallic = 0.0f;						// B : 金属製

		float				roughness = 1.0f;						// G : 粗さ
		int baseIndex = 0;
		int metaRoughnessIndex = 0;
		int emissiveIndex = 0;

		int normalIndex = 0;
		UINT startIndexLocation; // インデックスバッファの中の位置
		DirectX::XMFLOAT2 pad;
	};

	struct Vertex
	{
		DirectX::XMFLOAT2 uv;
	};

	// ---- レイワールドに登録するデータ ----
	struct Instance
	{
		DirectX::XMFLOAT4X4 worldMat = DXSM::Matrix::Identity;
		const BLAS* pBLAS = nullptr;

		// 頂点情報
		Handle<D3D12::SRV> vertexHandle = {};
		Handle<D3D12::SRV> indexHandle = {};

		// マテリアル
		//const Engine::Resource::Material* pMaterial = nullptr;
		std::vector<Material> submeshMaterial = {};

		// メッシュ
		const Resource::Mesh* pMesh = nullptr;
	};
}