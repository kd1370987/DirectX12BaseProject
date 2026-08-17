#pragma once
namespace Math
{
	/// <summary>
	/// 自作構造体
	/// 座標系 : 
	/// +X : Right
	/// +Y : UP
	/// -Z : Forward
	/// 左手座標系
	/// </summary>
	struct Vector3
	{
		//-----------------------------------------------------------------------------------------------------
		// データ
		float x;
		float y;
		float z;

		//-----------------------------------------------------------------------------------------------------
		// コンストラクタ
		constexpr Vector3() noexcept : x(0.0f), y(0.0f), z(0.0f) {}
		constexpr Vector3(float x, float y, float z) noexcept : x(x), y(y), z(z) {}

		//-----------------------------------------------------------------------------------------------------
		// Operators
		constexpr Vector3 operator+(const Vector3& a_other) const noexcept
		{
			return{ x + a_other.x,y + a_other.y,z + a_other.z };
		}
		constexpr Vector3 operator-(const Vector3& a_other) const noexcept
		{
			return{ x - a_other.x,y - a_other.y,z - a_other.z };
		}
		constexpr Vector3 operator*(float a_scalar) const noexcept
		{
			return{ x * a_scalar,y * a_scalar,z * a_scalar };
		}
		constexpr Vector3 operator/(float a_scalar) const noexcept
		{
			return{ x / a_scalar,y / a_scalar,z / a_scalar };
		}
		constexpr Vector3& operator+=(const Vector3& a_other) noexcept
		{
			x += a_other.x;
			y += a_other.y;
			z += a_other.z;

			return *this;
		}
		constexpr Vector3& operator-=(const Vector3& a_other) noexcept
		{
			x -= a_other.x;
			y -= a_other.y;
			z -= a_other.z;

			return *this;
		}
		constexpr Vector3& operator*=(float a_scalar) noexcept
		{
			x *= a_scalar;
			y *= a_scalar;
			z *= a_scalar;

			return *this;
		}
		constexpr Vector3& operator/=(float a_scalar) noexcept
		{
			x /= a_scalar;
			y /= a_scalar;
			z /= a_scalar;

			return *this;
		}
		constexpr Vector3 operator-() const noexcept
		{
			return { -x,-y,-z };
		}

		//-----------------------------------------------------------------------------------------------------
		// Length
		constexpr float LengthSquared() const noexcept
		{
			return x * x + y * y + z * z;
		}
		float Length() const noexcept
		{
			return std::sqrt(LengthSquared());
		}

		//-----------------------------------------------------------------------------------------------------
		// Normalize
		void Normalize() noexcept
		{

		}
		Vector3 Normalized() const noexcept
		{

		}

		//-----------------------------------------------------------------------------------------------------
		// Vector math
		constexpr float Dot(const Vector3&) const noexcept
		{

		}
		constexpr Vector3 Cross(const Vector3&) const noexcept
		{

		}

		//-----------------------------------------------------------------------------------------------------
		// Distance
		static float Distance(const Vector3& a_a, const Vector3& a_b) noexcept
		{

		}
		static constexpr float DistanceSquared(const Vector3& a_a, const Vector3& a_b) noexcept
		{

		}

		//-----------------------------------------------------------------------------------------------------
		// Interpolation
		static Vector3 Lerp(const Vector3& a_a, const Vector3& a_b, float a_t) noexcept
		{
		}

		//-----------------------------------------------------------------------------------------------------
		// Constants
		static constexpr Vector3 Zero()		noexcept { return { 0,0,0 }; }
		static constexpr Vector3 One()		noexcept { return { 1,1,1 }; }
		static constexpr Vector3 Up()		noexcept { return { 0,1,0 }; }
		static constexpr Vector3 Down()		noexcept { return { 0,-1,0 }; }
		static constexpr Vector3 Right()	noexcept { return { 1,0,0 }; }
		static constexpr Vector3 Left()		noexcept { return { -1,0,0 }; }
		static constexpr Vector3 Forward()	noexcept { return { 0,0,-1 }; }
		static constexpr Vector3 Backward() noexcept { return { 0,0,1 }; }


	};
}