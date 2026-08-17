#pragma once
namespace Math
{
	struct Color
	{
		// 実データ
		float r;
		float g;
		float b;
		float a;

		// コンストラクタ
		constexpr Color() noexcept : r(0.0f), g(0.0f), b(0.0f), a(0.0f) {}
		constexpr Color(float a_r, float a_g, float a_b, float a_a) noexcept : r(a_r), g(a_g), b(a_b), a(a_a) {}
	};
}