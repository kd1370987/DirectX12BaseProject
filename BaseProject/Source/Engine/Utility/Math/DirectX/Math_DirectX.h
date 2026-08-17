#pragma once

#include "../Vector/Vector2.h"
#include "../Vector/Vector3.h"
#include "../Vector/Vector4.h"

#include "../Matrix.h"
#include "../Quaternion.h"
#include "../Color.h"

//==========================================================================================
// DirectXMath との橋渡し。
//
// 型そのものの相互変換(XMFLOAT系 <-> Math系)は各構造体のコンストラクタと
// 変換演算子で暗黙に行えるので、ここに書くのは「演算子では書けないもの」だけ。
//   ・XMVECTOR / XMMATRIX(SIMD レジスタ型)との受け渡し
//   ・型が同じで意味が違うもの(Color <-> Vector4 など)の明示変換
//==========================================================================================
namespace Math::DX
{
	//-----------------------------------------------------------------------------------------------------
	// XMVECTOR / XMMATRIX との受け渡し
	//-----------------------------------------------------------------------------------------------------
	inline DirectX::XMVECTOR Load(const Vector2& a_value) noexcept
	{
		return DirectX::XMVectorSet(a_value.x, a_value.y, 0.0f, 0.0f);
	}
	inline DirectX::XMVECTOR Load(const Vector3& a_value) noexcept
	{
		return DirectX::XMVectorSet(a_value.x, a_value.y, a_value.z, 0.0f);
	}
	inline DirectX::XMVECTOR Load(const Vector4& a_value) noexcept
	{
		return DirectX::XMVectorSet(a_value.x, a_value.y, a_value.z, a_value.w);
	}
	inline DirectX::XMVECTOR Load(const Quaternion& a_value) noexcept
	{
		return DirectX::XMVectorSet(a_value.x, a_value.y, a_value.z, a_value.w);
	}
	inline DirectX::XMMATRIX Load(const Matrix& a_value) noexcept
	{
		return DirectX::XMLoadFloat4x4(reinterpret_cast<const DirectX::XMFLOAT4X4*>(&a_value));
	}

	inline Vector3 StoreVector3(DirectX::FXMVECTOR a_value) noexcept
	{
		DirectX::XMFLOAT3 _result = {};
		DirectX::XMStoreFloat3(&_result, a_value);
		return _result;
	}
	inline Vector4 StoreVector4(DirectX::FXMVECTOR a_value) noexcept
	{
		DirectX::XMFLOAT4 _result = {};
		DirectX::XMStoreFloat4(&_result, a_value);
		return _result;
	}
	inline Quaternion StoreQuaternion(DirectX::FXMVECTOR a_value) noexcept
	{
		DirectX::XMFLOAT4 _result = {};
		DirectX::XMStoreFloat4(&_result, a_value);
		return _result;
	}
	inline Matrix StoreMatrix(DirectX::FXMMATRIX a_value) noexcept
	{
		DirectX::XMFLOAT4X4 _result = {};
		DirectX::XMStoreFloat4x4(&_result, a_value);
		return _result;
	}

	//-----------------------------------------------------------------------------------------------------
	// 同じ4成分だが意味が違うもの。暗黙にすると事故るので明示
	//-----------------------------------------------------------------------------------------------------
	constexpr Vector4 ToVector4(const Color& a_value) noexcept
	{
		return { a_value.r, a_value.g, a_value.b, a_value.a };
	}
	constexpr Color ToColor(const Vector4& a_value) noexcept
	{
		return { a_value.x, a_value.y, a_value.z, a_value.w };
	}
}
