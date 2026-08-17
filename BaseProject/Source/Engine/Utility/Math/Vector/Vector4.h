#pragma once
namespace Math
{
	struct Vector4
	{
		// 実データ
		float x;
		float y;
		float z;
		float w;

		// コンストラクタ
		constexpr Vector4() noexcept : x(0.0f), y(0.0f), z(0.0f), w(0.0f) {}
		constexpr Vector4(float x, float y, float z, float w) noexcept : x(x), y(y), z(z), w(w) {}
	};
}