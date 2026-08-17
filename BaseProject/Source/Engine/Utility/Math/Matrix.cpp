#include "Matrix.h"

#include "Vector/Vector3.h"
#include "Quaternion.h"
#include "TRS.h"

//==========================================================================================
// 中身は DirectXMath の SIMD をそのまま借りる。
// 自前で展開しても得は無く、既存コード(DXSM 経由)と数値が変わらないほうが移行が安全。
//==========================================================================================
namespace Math
{
	namespace
	{
		inline DirectX::XMMATRIX Load(const Matrix& a_m) noexcept
		{
			return DirectX::XMLoadFloat4x4(reinterpret_cast<const DirectX::XMFLOAT4X4*>(&a_m));
		}
		inline Matrix Store(DirectX::FXMMATRIX a_m) noexcept
		{
			DirectX::XMFLOAT4X4 _result = {};
			DirectX::XMStoreFloat4x4(&_result, a_m);
			return _result;
		}
	}

	//--------------------------------------------------------------------------------------
	// Operators
	//--------------------------------------------------------------------------------------
	Matrix Matrix::operator*(const Matrix& a_other) const noexcept
	{
		// XMMatrixMultiply(a, b) は「a を適用してから b」。SimpleMath と同じ並び
		return Store(DirectX::XMMatrixMultiply(Load(*this), Load(a_other)));
	}

	Matrix& Matrix::operator*=(const Matrix& a_other) noexcept
	{
		*this = *this * a_other;
		return *this;
	}

	//--------------------------------------------------------------------------------------
	// 行の読み書き
	//--------------------------------------------------------------------------------------
	Vector3 Matrix::Right()       const noexcept { return { _11, _12, _13 }; }
	Vector3 Matrix::Up()          const noexcept { return { _21, _22, _23 }; }
	Vector3 Matrix::Forward()     const noexcept { return { _31, _32, _33 }; }
	Vector3 Matrix::Translation() const noexcept { return { _41, _42, _43 }; }

	void Matrix::SetTranslation(const Vector3& a_pos) noexcept
	{
		_41 = a_pos.x;
		_42 = a_pos.y;
		_43 = a_pos.z;
	}

	//--------------------------------------------------------------------------------------
	// 行列演算
	//--------------------------------------------------------------------------------------
	Matrix Matrix::Invert() const noexcept
	{
		return Store(DirectX::XMMatrixInverse(nullptr, Load(*this)));
	}

	Matrix Matrix::Transpose() const noexcept
	{
		return Store(DirectX::XMMatrixTranspose(Load(*this)));
	}

	bool Matrix::Decompose(Vector3& a_outScale, Quaternion& a_outRotation, Vector3& a_outTranslation) const noexcept
	{
		DirectX::XMVECTOR _scale = {};
		DirectX::XMVECTOR _rot   = {};
		DirectX::XMVECTOR _trans = {};

		if (!DirectX::XMMatrixDecompose(&_scale, &_rot, &_trans, Load(*this)))
		{
			return false;
		}

		DirectX::XMFLOAT3 _s = {};
		DirectX::XMFLOAT4 _r = {};
		DirectX::XMFLOAT3 _t = {};
		DirectX::XMStoreFloat3(&_s, _scale);
		DirectX::XMStoreFloat4(&_r, _rot);
		DirectX::XMStoreFloat3(&_t, _trans);

		a_outScale       = _s;
		a_outRotation    = _r;
		a_outTranslation = _t;
		return true;
	}

	//--------------------------------------------------------------------------------------
	// 生成
	//--------------------------------------------------------------------------------------
	Matrix Matrix::CreateTranslation(const Vector3& a_pos) noexcept
	{
		return Store(DirectX::XMMatrixTranslation(a_pos.x, a_pos.y, a_pos.z));
	}

	Matrix Matrix::CreateScale(const Vector3& a_scale) noexcept
	{
		return Store(DirectX::XMMatrixScaling(a_scale.x, a_scale.y, a_scale.z));
	}

	Matrix Matrix::CreateScale(float a_scale) noexcept
	{
		return Store(DirectX::XMMatrixScaling(a_scale, a_scale, a_scale));
	}

	Matrix Matrix::CreateScale(float a_x, float a_y, float a_z) noexcept
	{
		return Store(DirectX::XMMatrixScaling(a_x, a_y, a_z));
	}

	Matrix Matrix::CreateFromQuaternion(const Quaternion& a_rotation) noexcept
	{
		const DirectX::XMVECTOR _q =
			DirectX::XMVectorSet(a_rotation.x, a_rotation.y, a_rotation.z, a_rotation.w);
		return Store(DirectX::XMMatrixRotationQuaternion(_q));
	}

	Matrix Matrix::CreateFromYawPitchRoll(float a_yaw, float a_pitch, float a_roll) noexcept
	{
		return Store(DirectX::XMMatrixRotationRollPitchYaw(a_pitch, a_yaw, a_roll));
	}

	Matrix Matrix::CreateTRS(const Vector3& a_pos, const Quaternion& a_rotation, const Vector3& a_scale) noexcept
	{
		const DirectX::XMVECTOR _q =
			DirectX::XMVectorSet(a_rotation.x, a_rotation.y, a_rotation.z, a_rotation.w);

		const DirectX::XMMATRIX _m =
			DirectX::XMMatrixScaling(a_scale.x, a_scale.y, a_scale.z) *
			DirectX::XMMatrixRotationQuaternion(_q) *
			DirectX::XMMatrixTranslation(a_pos.x, a_pos.y, a_pos.z);

		return Store(_m);
	}

	Matrix Matrix::CreateLookAt(const Vector3& a_eye, const Vector3& a_target, const Vector3& a_up) noexcept
	{
		// このエンジンは左手系。右手系の LookAt を使うと向きが180度反転する
		const DirectX::XMVECTOR _eye    = DirectX::XMVectorSet(a_eye.x, a_eye.y, a_eye.z, 1.0f);
		const DirectX::XMVECTOR _target = DirectX::XMVectorSet(a_target.x, a_target.y, a_target.z, 1.0f);
		const DirectX::XMVECTOR _up     = DirectX::XMVectorSet(a_up.x, a_up.y, a_up.z, 0.0f);

		return Store(DirectX::XMMatrixLookAtLH(_eye, _target, _up));
	}

	Matrix Matrix::CreatePerspectiveFieldOfView(float a_fovY, float a_aspect, float a_nearZ, float a_farZ) noexcept
	{
		return Store(DirectX::XMMatrixPerspectiveFovLH(a_fovY, a_aspect, a_nearZ, a_farZ));
	}

	//--------------------------------------------------------------------------------------
	// TRS
	//--------------------------------------------------------------------------------------
	TRS Decompose(const Matrix& a_matrix) noexcept
	{
		TRS _result = {};
		a_matrix.Decompose(_result.scale, _result.rotation, _result.pos);
		return _result;
	}
}
