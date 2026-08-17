#pragma once
namespace Math
{
	struct Vector2
	{
		// 実データ
		float x;
		float y;

		// コンストラクタ
		constexpr Vector2() noexcept : x(0.0f), y(0.0f) {}
		constexpr Vector2(float x, float y) noexcept : x(x), y(y) {}
	};
}