#pragma once

// EngineCommon.h ではこちらが先に読まれるので、自分で引いておく
#include "../Utility/Math/Color.h"

namespace Engine
{
	namespace Color
	{
		constexpr Math::Color BLACK	= { 0.0f,0.0f,0.0f,1.0f };
		constexpr Math::Color WHITE	= { 1.0f,1.0f,1.0f,1.0f };
		constexpr Math::Color RED		= { 1.0f,0.0f,0.0f,1.0f };
		constexpr Math::Color GREEN	= { 0.0f,1.0f,0.0f,1.0f };
		constexpr Math::Color BLUE	= { 0.0f,0.0f,1.0f,1.0f };
	}

	namespace TexColor
	{
		constexpr Math::Color WHITE		= {255,255,255,255};
		constexpr Math::Color BLACK		= {0,0,0,255};
		constexpr Math::Color NORMAL	= {128,128,255,255};
		constexpr Math::Color ORM		= {0,255,255,255};
	}
}