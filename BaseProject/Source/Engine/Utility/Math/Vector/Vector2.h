#pragma once

//==========================================================================================
// Math::Vector2
//
// ECS のコンポーネントにそのまま置ける POD のベクトル。
//
// ・DirectXMath / SimpleMath とは暗黙に相互変換できる。
//   変換演算子は DXSM 側にだけ用意してある。DXSM::Vector2 は XMFLOAT2 の派生なので、
//   これ1つで「XMFLOAT2 を取る所」「DXSM::Vector2 を取る所」の両方へ渡せる。
//   XMFLOAT2 用の変換演算子も足すと、const XMFLOAT2& へ渡すときに
//   どちらの経路でも行けてしまい曖昧になるため、あえて片方だけにしている。
// ・DirectXMath は Pch.h で必ず読み込まれている前提。
//==========================================================================================
namespace Math
{
	struct Vector2
	{
		//-----------------------------------------------------------------------------------------------------
		// データ
		float x;
		float y;

		//-----------------------------------------------------------------------------------------------------
		// コンストラクタ
		constexpr Vector2() noexcept : x(0.0f), y(0.0f) {}
		constexpr Vector2(float a_x, float a_y) noexcept : x(a_x), y(a_y) {}
		explicit constexpr Vector2(float a_value) noexcept : x(a_value), y(a_value) {}

		//-----------------------------------------------------------------------------------------------------
		// DirectX 相互変換
		constexpr Vector2(const DirectX::XMFLOAT2& a_value) noexcept : x(a_value.x), y(a_value.y) {}
		operator DXSM::Vector2() const noexcept { return DXSM::Vector2(x, y); }

		//-----------------------------------------------------------------------------------------------------
		// Operators
		constexpr Vector2 operator+(const Vector2& a_other) const noexcept { return { x + a_other.x, y + a_other.y }; }
		constexpr Vector2 operator-(const Vector2& a_other) const noexcept { return { x - a_other.x, y - a_other.y }; }
		constexpr Vector2 operator*(const Vector2& a_other) const noexcept { return { x * a_other.x, y * a_other.y }; }
		constexpr Vector2 operator/(const Vector2& a_other) const noexcept { return { x / a_other.x, y / a_other.y }; }
		constexpr Vector2 operator*(float a_scalar) const noexcept { return { x * a_scalar, y * a_scalar }; }
		constexpr Vector2 operator/(float a_scalar) const noexcept { return { x / a_scalar, y / a_scalar }; }
		constexpr Vector2 operator-() const noexcept { return { -x, -y }; }

		constexpr Vector2& operator+=(const Vector2& a_other) noexcept { x += a_other.x; y += a_other.y; return *this; }
		constexpr Vector2& operator-=(const Vector2& a_other) noexcept { x -= a_other.x; y -= a_other.y; return *this; }
		constexpr Vector2& operator*=(const Vector2& a_other) noexcept { x *= a_other.x; y *= a_other.y; return *this; }
		constexpr Vector2& operator*=(float a_scalar) noexcept { x *= a_scalar; y *= a_scalar; return *this; }
		constexpr Vector2& operator/=(float a_scalar) noexcept { x /= a_scalar; y /= a_scalar; return *this; }

		constexpr bool operator==(const Vector2& a_other) const noexcept { return x == a_other.x && y == a_other.y; }
		constexpr bool operator!=(const Vector2& a_other) const noexcept { return !(*this == a_other); }

		//-----------------------------------------------------------------------------------------------------
		// Length
		constexpr float LengthSquared() const noexcept { return x * x + y * y; }
		float Length() const noexcept { return std::sqrt(LengthSquared()); }

		//-----------------------------------------------------------------------------------------------------
		// Normalize
		// 長さ 0 のときは 0 のまま(0除算で NaN を撒かない)
		void Normalize() noexcept;
		Vector2 Normalized() const noexcept;

		//-----------------------------------------------------------------------------------------------------
		// Vector math
		constexpr float Dot(const Vector2& a_other) const noexcept { return x * a_other.x + y * a_other.y; }
		// 2Dの外積(スカラー)。左右どちら側にあるかの判定に使う
		constexpr float Cross(const Vector2& a_other) const noexcept { return x * a_other.y - y * a_other.x; }

		//-----------------------------------------------------------------------------------------------------
		// Distance
		static float Distance(const Vector2& a_a, const Vector2& a_b) noexcept { return (a_b - a_a).Length(); }
		static constexpr float DistanceSquared(const Vector2& a_a, const Vector2& a_b) noexcept { return (a_b - a_a).LengthSquared(); }

		//-----------------------------------------------------------------------------------------------------
		// Interpolation
		static constexpr Vector2 Lerp(const Vector2& a_a, const Vector2& a_b, float a_t) noexcept
		{
			return { a_a.x + (a_b.x - a_a.x) * a_t, a_a.y + (a_b.y - a_a.y) * a_t };
		}

		//-----------------------------------------------------------------------------------------------------
		// Constants
		static constexpr Vector2 Zero()  noexcept { return { 0.0f, 0.0f }; }
		static constexpr Vector2 One()   noexcept { return { 1.0f, 1.0f }; }
		static constexpr Vector2 UnitX() noexcept { return { 1.0f, 0.0f }; }
		static constexpr Vector2 UnitY() noexcept { return { 0.0f, 1.0f }; }
	};

	// スカラーが左に来る形
	constexpr Vector2 operator*(float a_scalar, const Vector2& a_value) noexcept { return a_value * a_scalar; }
}

//==========================================================================================
// 保存データとの互換チェック
//------------------------------------------------------------------------------------------
// Archive のバイナリは構造体をそのまま memcpy している。
// XMFLOAT系と1バイトでもズレると、既存の .ob* が静かに壊れる(例外も出ない)ので、
// 並びが変わったらここでコンパイルエラーにする。
//==========================================================================================
static_assert(sizeof(Math::Vector2) == sizeof(DirectX::XMFLOAT2), "Math::Vector2 のサイズが XMFLOAT2 と違う");
static_assert(offsetof(Math::Vector2, y) == offsetof(DirectX::XMFLOAT2, y), "Math::Vector2 の並びが XMFLOAT2 と違う");
