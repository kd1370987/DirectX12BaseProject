#pragma once

struct Texture
{
	ComPtr<ID3D12Resource> cpResource = nullptr;			// テクスチャリソース
	D3D12_RESOURCE_DESC* pDesc = nullptr;							// テクスチャの仕様書
};
