#include "Vector4.h"

#include "Vector3.h"
#include "../Matrix.h"

namespace Math
{
	void Vector4::Normalize() noexcept
	{
		const float _lenSq = LengthSquared();
		if (_lenSq <= 1e-12f) return;	// 長さ0は触らない(NaNを撒かない)

		const float _inv = 1.0f / std::sqrt(_lenSq);
		x *= _inv;
		y *= _inv;
		z *= _inv;
		w *= _inv;
	}

	Vector4 Vector4::Normalized() const noexcept
	{
		Vector4 _result = *this;
		_result.Normalize();
		return _result;
	}

	Vector4 Vector4::Transform(const Vector4& a_value, const Matrix& a_matrix) noexcept
	{
		const DirectX::XMVECTOR _v = DirectX::XMVectorSet(a_value.x, a_value.y, a_value.z, a_value.w);
		const DirectX::XMMATRIX _m = DirectX::XMLoadFloat4x4(
			reinterpret_cast<const DirectX::XMFLOAT4X4*>(&a_matrix));

		DirectX::XMFLOAT4 _result = {};
		DirectX::XMStoreFloat4(&_result, DirectX::XMVector4Transform(_v, _m));
		return _result;
	}

	Vector4 Vector4::Transform(const Vector3& a_value, const Matrix& a_matrix) noexcept
	{
		// w = 1 の座標として掛ける。射影行列を掛けたあとの w が欲しい
		// (スクリーン射影でカメラ後方を弾くのに使う)ので Vector3 版とは別に用意する
		const DirectX::XMVECTOR _v = DirectX::XMVectorSet(a_value.x, a_value.y, a_value.z, 1.0f);
		const DirectX::XMMATRIX _m = DirectX::XMLoadFloat4x4(
			reinterpret_cast<const DirectX::XMFLOAT4X4*>(&a_matrix));

		DirectX::XMFLOAT4 _result = {};
		DirectX::XMStoreFloat4(&_result, DirectX::XMVector4Transform(_v, _m));
		return _result;
	}
}
