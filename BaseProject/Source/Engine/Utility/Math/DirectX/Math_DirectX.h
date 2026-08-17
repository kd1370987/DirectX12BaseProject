#pragma once

#include "../Vector/Vector2.h"
#include "../Vector/Vector3.h"
#include "../Vector/Vector4.h"

#include "../Matrix.h"
#include "../Quaternion.h"

/// <summary>
/// DirectXMathが読み込まれた際にインクルードして
/// DirectXMathと互換性を持たせるためのもの
/// </summary>
namespace Math::DX
{
	//-----------------------------------------------------------------------------------------------------
	// 型変換
	//-----------------------------------------------------------------------------------------------------
	inline Math::Vector2 FromDX(const DirectX::XMFLOAT2& a_value) noexcept
	{
		return { a_value.x,a_value.y};
	}
	inline Math::Vector3 FromDX(const DirectX::XMFLOAT3& a_value) noexcept
	{
		return { a_value.x,a_value.y,a_value.z };
	}
	inline Math::Vector4 FromDX(const DirectX::XMFLOAT4& a_value) noexcept
	{
		return { a_value.x,a_value.y,a_value.z ,a_value.w};
	}
}