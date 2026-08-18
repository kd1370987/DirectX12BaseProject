#include "Vector3.h"

#include "../Quaternion.h"
#include "../Matrix.h"

//==========================================================================================
// 変換系は DirectXMath の SIMD をそのまま借りる。
// 自前で展開しても得は無く、既存コード(DXSM 経由)と数値が変わらないほうが移行が安全。
//==========================================================================================
namespace Math
{
	void Vector3::Normalize() noexcept
	{
		const float _lenSq = LengthSquared();
		if (_lenSq <= 1e-12f) return;	// 長さ0は触らない(NaNを撒かない)

		const float _inv = 1.0f / std::sqrt(_lenSq);
		x *= _inv;
		y *= _inv;
		z *= _inv;
	}

	Vector3 Vector3::Normalized() const noexcept
	{
		Vector3 _result = *this;
		_result.Normalize();
		return _result;
	}

	Vector3 Vector3::Transform(const Vector3& a_value, const Quaternion& a_rotation) noexcept
	{
		const DirectX::XMVECTOR _v = DirectX::XMVectorSet(a_value.x, a_value.y, a_value.z, 0.0f);
		const DirectX::XMVECTOR _q = DirectX::XMVectorSet(a_rotation.x, a_rotation.y, a_rotation.z, a_rotation.w);

		DirectX::XMFLOAT3 _result = {};
		DirectX::XMStoreFloat3(&_result, DirectX::XMVector3Rotate(_v, _q));
		return _result;
	}

	Vector3 Vector3::Transform(const Vector3& a_value, const Matrix& a_matrix) noexcept
	{
		const DirectX::XMVECTOR _v = DirectX::XMVectorSet(a_value.x, a_value.y, a_value.z, 0.0f);
		const DirectX::XMMATRIX _m = DirectX::XMLoadFloat4x4(
			reinterpret_cast<const DirectX::XMFLOAT4X4*>(&a_matrix));

		DirectX::XMFLOAT3 _result = {};
		DirectX::XMStoreFloat3(&_result, DirectX::XMVector3Transform(_v, _m));
		return _result;
	}

	Vector3 Vector3::TransformNormal(const Vector3& a_value, const Matrix& a_matrix) noexcept
	{
		const DirectX::XMVECTOR _v = DirectX::XMVectorSet(a_value.x, a_value.y, a_value.z, 0.0f);
		const DirectX::XMMATRIX _m = DirectX::XMLoadFloat4x4(
			reinterpret_cast<const DirectX::XMFLOAT4X4*>(&a_matrix));

		DirectX::XMFLOAT3 _result = {};
		DirectX::XMStoreFloat3(&_result, DirectX::XMVector3TransformNormal(_v, _m));
		return _result;
	}
}
