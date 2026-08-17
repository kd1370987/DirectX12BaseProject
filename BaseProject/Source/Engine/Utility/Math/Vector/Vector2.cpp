#include "Vector2.h"

namespace Math
{
	void Vector2::Normalize() noexcept
	{
		const float _lenSq = LengthSquared();
		if (_lenSq <= 1e-12f) return;	// 長さ0は触らない(NaNを撒かない)

		const float _inv = 1.0f / std::sqrt(_lenSq);
		x *= _inv;
		y *= _inv;
	}

	Vector2 Vector2::Normalized() const noexcept
	{
		Vector2 _result = *this;
		_result.Normalize();
		return _result;
	}
}
