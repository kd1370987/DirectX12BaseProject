#pragma once

namespace Engine::Raytracing
{
	// メッシュごと
	struct Instance
	{
		DirectX::XMFLOAT4X4 worldMat = DXSM::Matrix::Identity;
		BLAS* pBLAS = nullptr;
		Engine::Resource::Handle<SRV> vertexHandle = {};
		Engine::Resource::Handle<SRV> indexHandle = {};
	};
}