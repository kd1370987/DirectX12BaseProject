#pragma once

namespace Collider
{
	struct Result
	{
		DirectX::XMFLOAT3 hitPos = { 0.0f,0.0f,0.0f };
		DirectX::XMFLOAT3 hitNormal = { 0.0f,0.0f,0.0f };
		float hitDistance = 0.0f;
		bool isHit = false;
	};

	struct RayInfo
	{
		DirectX::XMFLOAT3 origin = { 0.0f,0.0f,0.0f };
		DirectX::XMFLOAT3 direction = { 0.0f,0.0f,1.0f };
		float maxDistance = 1000.0f;
	};

	struct MeshInfo
	{
		const DirectX::XMFLOAT3* pVertices = nullptr;
		uint32_t triangleCount = 0;
	};

	bool RayVsMesh(
		RayInfo& a_ray,
		DirectX::XMVECTOR v0,
		DirectX::XMVECTOR v1,
		DirectX::XMVECTOR v2,
		float& a_outT
	);


}