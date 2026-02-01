#pragma once

#include "D3DObject/Viewport/Viewport.h"
#include "D3DObject/ScissorRectangle/ScissorRectangle.h"

enum class RangeType
{
	CBV,
	SRV,
	UAV,
};

enum class RootParameterType
{
	DescriptorTable,
	RootCBV,
};

struct SRVViewInit
{
	ID3D12Resource* pResource = nullptr;
	D3D12_SHADER_RESOURCE_VIEW_DESC* pDesc = nullptr;
};

struct UAVViewInit
{
	ID3D12Resource* pResource = nullptr;
	D3D12_UNORDERED_ACCESS_VIEW_DESC* pDesc = nullptr;
};

struct RTVHandle
{
	UINT index = INVALID_INDEX;
};

struct DSVHandle
{
	UINT index = INVALID_INDEX;
};

struct CBVHandle
{
	UINT index = INVALID_INDEX;
};

using SRVHandle = Storage::Range;
using UAVHandle = Storage::Range;
