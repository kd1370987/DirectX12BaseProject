#pragma once

struct UIComponent
{
	// テクスチャID
	Engine::Resource::ID texID = Engine::Resource::Limits::INVALID_ID;

	//Storage::Range srvRange = {};
	//Engine::Resource::Handle<SRV> srvHandle = {};
	Engine::Resource::Handle<Engine::Resource::Texture> texHandle = {};

	// UVオフセットとタイル
	DirectX::XMFLOAT4 uvOffsetTiling = { 0.0f,0.0f,1.0f,1.0f };
	// 色
	DirectX::XMFLOAT4 color = { 1.0f,1.0f,1.0f,1.0f };
};