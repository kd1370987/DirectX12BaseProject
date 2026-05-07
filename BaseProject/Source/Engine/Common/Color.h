#pragma once
namespace Engine
{
	namespace Color
	{
		constexpr DirectX::XMFLOAT4 BLACK	= { 0.0f,0.0f,0.0f,1.0f };
		constexpr DirectX::XMFLOAT4 WHITE	= { 1.0f,1.0f,1.0f,1.0f };
		constexpr DirectX::XMFLOAT4 RED		= { 1.0f,0.0f,0.0f,1.0f };
		constexpr DirectX::XMFLOAT4 GREEN	= { 0.0f,1.0f,0.0f,1.0f };
		constexpr DirectX::XMFLOAT4 BLUE	= { 0.0f,0.0f,1.0f,1.0f };
	}

	namespace TexColor
	{
		constexpr DXSM::Color WHITE		= {255,255,255,255};
		constexpr DXSM::Color BLACK		= {0,0,0,255};
		constexpr DXSM::Color NORMAL	= {128,128,255,255};
		constexpr DXSM::Color ORM		= {0,255,255,255};
	}
}