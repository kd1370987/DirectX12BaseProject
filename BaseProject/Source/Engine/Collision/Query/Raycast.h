#pragma once

struct Result
{
	ECS::Entity hitEntity = ECS::Limits::INVALID_ENTITY;
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