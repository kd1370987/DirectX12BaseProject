#pragma once

namespace Engine::Raytracing
{
	// ---- 構造体バッファ用データ ----
	// インスタンスごとのデータ
	struct InstanceData
	{
		UINT vertexSRVIndex;
		UINT indexSRVIndex;
	};
	// マテリアルSRVに対しての個別設定データ
	struct Material
	{
		DXSM::Vector4 baseColor = { 1,1,1,1 };
		float						metallic = 0.0f;						// B : 金属製
		float						roughness = 1.0f;						// G : 粗さ
		DirectX::XMFLOAT3			emissive = { 1.0f,1.0f,1.0f };

		int baseIndex = 0;
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
		Engine::Resource::Handle<D3D12::SRV> vertexHandle = {};
		Engine::Resource::Handle<D3D12::SRV> indexHandle = {};

		// マテリアル
		//const Engine::Resource::Material* pMaterial = nullptr;
		std::vector<Material> submeshMaterial = {};

		// メッシュ
		const Resource::Mesh* pMesh = nullptr;
	};
}