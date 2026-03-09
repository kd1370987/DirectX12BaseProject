#pragma once

#include "D3DObject/Viewport/Viewport.h"
#include "D3DObject/ScissorRectangle/ScissorRectangle.h"

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

//struct RTVHandle
//{
//	UINT index = INVALID_INDEX;
//};

struct DSVHandle
{
	UINT index = INVALID_INDEX;
};

struct CBVHandle
{
	UINT index = INVALID_INDEX;
};

struct CBV {};
struct SRV {};
struct UAV {};
struct RTV {};
struct DSV {};

using SRVHandle = Storage::Range;
using UAVHandle = Storage::Range;

#include "Engine/D3D12/D3DObject/DescriptorHeap/DescriptorHeap.h"
