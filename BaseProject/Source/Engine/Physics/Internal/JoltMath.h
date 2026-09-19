#pragma once

#include <Jolt/Jolt.h>

#include "Engine/Utility/Math/Vector/Vector3.h"
#include "Engine/Utility/Math/Quaternion.h"
#include "Engine/Utility/Math/Matrix.h"

//==========================================================================================
// 自作 Math 型 <-> Jolt の型の変換。
//
// Physics の外へは Math 型だけを見せ、JPH:: の型はこのディレクトリ以下に閉じ込める。
// 変換はここに集め、呼び出し側で成分を並べ替えない
//==========================================================================================
namespace Engine::Physics::Internal
{
	//-----------------------------------------------------------------------------------------------------
	// Math -> Jolt
	//-----------------------------------------------------------------------------------------------------
	inline JPH::Vec3 ToJolt(const Math::Vector3& a_value) noexcept
	{
		return JPH::Vec3(a_value.x, a_value.y, a_value.z);
	}
	inline JPH::RVec3 ToJoltR(const Math::Vector3& a_value) noexcept
	{
		return JPH::RVec3(a_value.x, a_value.y, a_value.z);
	}
	inline JPH::Quat ToJolt(const Math::Quaternion& a_value) noexcept
	{
		return JPH::Quat(a_value.x, a_value.y, a_value.z, a_value.w);
	}
	inline JPH::Mat44 ToJolt(const Math::Matrix& a_value) noexcept
	{
		return JPH::Mat44(
			JPH::Vec4(a_value._11, a_value._12, a_value._13, a_value._14),
			JPH::Vec4(a_value._21, a_value._22, a_value._23, a_value._24),
			JPH::Vec4(a_value._31, a_value._32, a_value._33, a_value._34),
			JPH::Vec4(a_value._41, a_value._42, a_value._43, a_value._44));
	}

	//-----------------------------------------------------------------------------------------------------
	// Jolt -> Math
	//-----------------------------------------------------------------------------------------------------
	inline Math::Vector3 ToMath(JPH::Vec3Arg a_value) noexcept
	{
		return Math::Vector3(a_value.GetX(), a_value.GetY(), a_value.GetZ());
	}
	inline Math::Quaternion ToMath(JPH::QuatArg a_value) noexcept
	{
		return Math::Quaternion(a_value.GetX(), a_value.GetY(), a_value.GetZ(), a_value.GetW());
	}
	inline Math::Matrix ToMath(JPH::Mat44Arg a_value) noexcept
	{
		const JPH::Vec4 _c0 = a_value.GetColumn4(0);
		const JPH::Vec4 _c1 = a_value.GetColumn4(1);
		const JPH::Vec4 _c2 = a_value.GetColumn4(2);
		const JPH::Vec4 _c3 = a_value.GetColumn4(3);
		return Math::Matrix(
			_c0.GetX(), _c0.GetY(), _c0.GetZ(), _c0.GetW(),
			_c1.GetX(), _c1.GetY(), _c1.GetZ(), _c1.GetW(),
			_c2.GetX(), _c2.GetY(), _c2.GetZ(), _c2.GetW(),
			_c3.GetX(), _c3.GetY(), _c3.GetZ(), _c3.GetW());
	}
}
