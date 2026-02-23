#pragma once

struct UIComponent
{
	// テクスチャID
	Resource::ID texID = Resource::Limits::INVALID_ID;

	Storage::Range srvRange = {};

	// UVオフセットとタイル
	DirectX::XMFLOAT4 uvOffsetTiling = { 0.0f,0.0f,1.0f,1.0f };
	// 色
	DirectX::XMFLOAT4 color = { 1.0f,1.0f,1.0f,1.0f };
};