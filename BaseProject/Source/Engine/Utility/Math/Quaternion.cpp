#include "Quaternion.h"

#include "Vector/Vector3.h"
#include "Matrix.h"

//==========================================================================================
// 中身は DirectXMath の SIMD をそのまま借りる。
// 自前で展開しても得は無く、既存コード(DXSM 経由)と数値が変わらないほうが移行が安全。
//==========================================================================================
namespace Math
{
	namespace
	{
		inline DirectX::XMVECTOR Load(const Quaternion& a_q) noexcept
		{
			return DirectX::XMVectorSet(a_q.x, a_q.y, a_q.z, a_q.w);
		}
		inline Quaternion Store(DirectX::FXMVECTOR a_v) noexcept
		{
			DirectX::XMFLOAT4 _result = {};
			DirectX::XMStoreFloat4(&_result, a_v);
			return _result;
		}
	}

	Quaternion Quaternion::operator*(const Quaternion& a_other) const noexcept
	{
		// XMQuaternionMultiply(a, b) は「a を適用してから b」。SimpleMath と同じ並び
		return Store(DirectX::XMQuaternionMultiply(Load(*this), Load(a_other)));
	}

	Quaternion& Quaternion::operator*=(const Quaternion& a_other) noexcept
	{
		*this = *this * a_other;
		return *this;
	}

	void Quaternion::Normalize() noexcept
	{
		const float _lenSq = LengthSquared();

		// 長さ 0 は無回転へ倒す。そのまま行列にすると全成分 0 の行列ができて
		// 描画が消えるので、0 のままにはしない(Vector とは扱いが違う)
		if (_lenSq <= 1e-12f)
		{
			*this = Identity();
			return;
		}

		const float _inv = 1.0f / std::sqrt(_lenSq);
		x *= _inv;
		y *= _inv;
		z *= _inv;
		w *= _inv;
	}

	Quaternion Quaternion::Normalized() const noexcept
	{
		Quaternion _result = *this;
		_result.Normalize();
		return _result;
	}

	Quaternion Quaternion::Inverse() const noexcept
	{
		return Store(DirectX::XMQuaternionInverse(Load(*this)));
	}

	Quaternion Quaternion::CreateFromYawPitchRoll(float a_yaw, float a_pitch, float a_roll) noexcept
	{
		return Store(DirectX::XMQuaternionRotationRollPitchYaw(a_pitch, a_yaw, a_roll));
	}

	Quaternion Quaternion::CreateFromAxisAngle(const Vector3& a_axis, float a_angle) noexcept
	{
		const DirectX::XMVECTOR _axis = DirectX::XMVectorSet(a_axis.x, a_axis.y, a_axis.z, 0.0f);
		return Store(DirectX::XMQuaternionRotationAxis(_axis, a_angle));
	}

	Quaternion Quaternion::CreateFromRotationMatrix(const Matrix& a_matrix) noexcept
	{
		const DirectX::XMMATRIX _m = DirectX::XMLoadFloat4x4(
			reinterpret_cast<const DirectX::XMFLOAT4X4*>(&a_matrix));
		return Store(DirectX::XMQuaternionRotationMatrix(_m));
	}

	Quaternion Quaternion::LookRotation(const Vector3& a_forward, const Vector3& a_up) noexcept
	{
		// 向きが決まらない入力は無回転を返す(NaN を撒かない)
		Vector3 _forward = a_forward.Normalized();
		if (_forward.LengthSquared() <= 1e-8f) return Identity();

		Vector3 _right = a_up.Cross(_forward);
		if (_right.LengthSquared() <= 1e-8f)
		{
			// 前方と上がほぼ平行。適当な直交軸で作り直す
			const Vector3 _ref = (std::fabs(_forward.y) > 0.99f) ? Vector3::Right() : Vector3::Up();
			_right = _ref.Cross(_forward);
		}
		_right.Normalize();

		const Vector3 _upOrtho = _forward.Cross(_right);

		// 左手系の基底をそのまま行列にして回転を取り出す(行が各軸)
		Matrix _rotMat = Matrix::Identity();
		_rotMat._11 = _right.x;    _rotMat._12 = _right.y;    _rotMat._13 = _right.z;
		_rotMat._21 = _upOrtho.x;  _rotMat._22 = _upOrtho.y;  _rotMat._23 = _upOrtho.z;
		_rotMat._31 = _forward.x;  _rotMat._32 = _forward.y;  _rotMat._33 = _forward.z;

		return CreateFromRotationMatrix(_rotMat);
	}

	Vector3 Quaternion::ToEuler() const noexcept
	{
		// 回転行列の成分から取り出す。ジンバルロック(cy がほぼ 0)のときは
		// Yaw を 0 に倒して Roll 側へ寄せる(SimpleMath と同じ扱い)
		const float _xx = x * x;
		const float _yy = y * y;
		const float _zz = z * z;

		const float _m31 = 2.0f * x * z + 2.0f * y * w;
		const float _m32 = 2.0f * y * z - 2.0f * x * w;
		const float _m33 = 1.0f - 2.0f * _xx - 2.0f * _yy;

		const float _cy = std::sqrt(_m33 * _m33 + _m31 * _m31);
		const float _cx = std::atan2(-_m32, _cy);

		if (_cy > 16.0f * FLT_EPSILON)
		{
			const float _m12 = 2.0f * x * y + 2.0f * z * w;
			const float _m22 = 1.0f - 2.0f * _xx - 2.0f * _zz;

			return { _cx, std::atan2(_m31, _m33), std::atan2(_m12, _m22) };
		}

		const float _m11 = 1.0f - 2.0f * _yy - 2.0f * _zz;
		const float _m21 = 2.0f * x * y - 2.0f * z * w;

		return { _cx, 0.0f, std::atan2(-_m21, _m11) };
	}

	Quaternion Quaternion::Slerp(const Quaternion& a_a, const Quaternion& a_b, float a_t) noexcept
	{
		return Store(DirectX::XMQuaternionSlerp(Load(a_a), Load(a_b), a_t));
	}
}
