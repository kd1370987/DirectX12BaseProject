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



