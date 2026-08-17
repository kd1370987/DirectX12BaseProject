#pragma once

#include "../Vector/Vector2.h"
#include "../Vector/Vector3.h"
#include "../Vector/Vector4.h"

#include "../Matrix.h"
#include "../Quaternion.h"
#include "../Color.h"

//==========================================================================================
// DirectXTK(SimpleMath)との橋渡し。
//
// Math -> DXSM は各構造体の変換演算子、DXSM -> Math は XMFLOAT系のコンストラクタで
// 暗黙に通る(DXSM の型は XMFLOAT系の派生なので、基底として受け取れる)。
// ここに置くのは、暗黙変換だけでは書けない・書くと分かりにくい所の明示版だけ。
//
// ※ 新しく書くコードは Math 側で完結させること。ここは既存の DXSM を使う
//    エンジン内部(Graphics / Editor / Collision)との境界のためのもの。
//==========================================================================================
namespace Math::DXSMBridge
{
	//-----------------------------------------------------------------------------------------------------
	// Math -> SimpleMath
	//-----------------------------------------------------------------------------------------------------
	inline DXSM::Vector2    To(const Vector2& a_value)    noexcept { return DXSM::Vector2(a_value.x, a_value.y); }
	inline DXSM::Vector3    To(const Vector3& a_value)    noexcept { return DXSM::Vector3(a_value.x, a_value.y, a_value.z); }
	inline DXSM::Vector4    To(const Vector4& a_value)    noexcept { return DXSM::Vector4(a_value.x, a_value.y, a_value.z, a_value.w); }
	inline DXSM::Quaternion To(const Quaternion& a_value) noexcept { return DXSM::Quaternion(a_value.x, a_value.y, a_value.z, a_value.w); }
	inline DXSM::Color      To(const Color& a_value)      noexcept { return DXSM::Color(a_value.r, a_value.g, a_value.b, a_value.a); }
	inline DXSM::Matrix     To(const Matrix& a_value)     noexcept
	{
		return DXSM::Matrix(
			a_value._11, a_value._12, a_value._13, a_value._14,
			a_value._21, a_value._22, a_value._23, a_value._24,
			a_value._31, a_value._32, a_value._33, a_value._34,
			a_value._41, a_value._42, a_value._43, a_value._44);
	}

	//-----------------------------------------------------------------------------------------------------
	// SimpleMath -> Math
	//-----------------------------------------------------------------------------------------------------
	inline Vector2    From(const DXSM::Vector2& a_value)    noexcept { return { a_value.x, a_value.y }; }
	inline Vector3    From(const DXSM::Vector3& a_value)    noexcept { return { a_value.x, a_value.y, a_value.z }; }
	inline Vector4    From(const DXSM::Vector4& a_value)    noexcept { return { a_value.x, a_value.y, a_value.z, a_value.w }; }
	inline Quaternion From(const DXSM::Quaternion& a_value) noexcept { return { a_value.x, a_value.y, a_value.z, a_value.w }; }
	inline Color      From(const DXSM::Color& a_value)      noexcept { return { a_value.x, a_value.y, a_value.z, a_value.w }; }
	inline Matrix     From(const DXSM::Matrix& a_value)     noexcept
	{
		return Matrix(
			a_value._11, a_value._12, a_value._13, a_value._14,
			a_value._21, a_value._22, a_value._23, a_value._24,
			a_value._31, a_value._32, a_value._33, a_value._34,
			a_value._41, a_value._42, a_value._43, a_value._44);
	}
}
