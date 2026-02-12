#pragma once

constexpr UINT MAX_NODEINDEX = 100;

struct NodePoseComponent
{
	DirectX::XMFLOAT4X4 local[MAX_NODEINDEX];
	DirectX::XMFLOAT4X4 world[MAX_NODEINDEX];
	uint16_t nodeCount;
};