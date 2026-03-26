#pragma once

namespace Engine::Raytracing
{
	// メッシュごと
	struct Instance
	{
		DirectX::XMFLOAT4X4 worldMat = DXSM::Matrix::Identity;
		BLAS* pBLAS = nullptr;

		// 頂点情報
		Engine::Resource::Handle<SRV> vertexHandle = {};
		Engine::Resource::Handle<SRV> indexHandle = {};

		// マテリアル
		const Engine::Resource::Material* pMaterial = nullptr;
	};
}