#pragma once

//==========================================================================================
// Math::Vector4
//
// ECS のコンポーネントにそのまま置ける POD のベクトル。
// 同次座標(クリップ座標)や、シェーダーへ渡す4成分の値に使う。
//
// ・DXSM::Vector4 は XMFLOAT4 の派生なので、変換演算子は DXSM 側にだけ用意する
//   (詳しくは Vector3.h のコメント)。
// ・Matrix に依存する処理は前方宣言にして .cpp で実装する。
//==========================================================================================
namespace Math
{
	struct Matrix;
	struct Vector3;

	struct Vector4
	{
		//-----------------------------------------------------------------------------------------------------
		// データ
		float x;
		float y;
		float z;
		float w;

		//-----------------------------------------------------------------------------------------------------
		// コンストラクタ
		constexpr Vector4() noexcept : x(0.0f), y(0.0f), z(0.0f), w(0.0f) {}
		constexpr Vector4(float a_x, float a_y, float a_z, float a_w) noexcept : x(a_x), y(a_y), z(a_z), w(a_w) {}
		explicit constexpr Vector4(float a_value) noexcept : x(a_value), y(a_value), z(a_value), w(a_value) {}

		//-----------------------------------------------------------------------------------------------------
		// DirectX 相互変換
		constexpr Vector4(const DirectX::XMFLOAT4& a_value) noexcept
			: x(a_value.x), y(a_value.y), z(a_value.z), w(a_value.w) {}
		operator DXSM::Vector4() const noexcept { return DXSM::Vector4(x, y, z, w); }

		//-----------------------------------------------------------------------------------------------------
		// Operators
		constexpr Vector4 operator+(const Vector4& a_other) const noexcept { return { x + a_other.x, y + a_other.y, z + a_other.z, w + a_other.w }; }
		constexpr Vector4 operator-(const Vector4& a_other) const noexcept { return { x - a_other.x, y - a_other.y, z - a_other.z, w - a_other.w }; }
		constexpr Vector4 operator*(const Vector4& a_other) const noexcept { return { x * a_other.x, y * a_other.y, z * a_other.z, w * a_other.w }; }
		constexpr Vector4 operator*(float a_scalar) const noexcept { return { x * a_scalar, y * a_scalar, z * a_scalar, w * a_scalar }; }
		constexpr Vector4 operator/(float a_scalar) const noexcept { return { x / a_scalar, y / a_scalar, z / a_scalar, w / a_scalar }; }
		constexpr Vector4 operator-() const noexcept { return { -x, -y, -z, -w }; }

		constexpr Vector4& operator+=(const Vector4& a_other) noexcept { x += a_other.x; y += a_other.y; z += a_other.z; w += a_other.w; return *this; }
		constexpr Vector4& operator-=(const Vector4& a_other) noexcept { x -= a_other.x; y -= a_other.y; z -= a_other.z; w -= a_other.w; return *this; }
		constexpr Vector4& operator*=(float a_scalar) noexcept { x *= a_scalar; y *= a_scalar; z *= a_scalar; w *= a_scalar; return *this; }
		constexpr Vector4& operator/=(float a_scalar) noexcept { x /= a_scalar; y /= a_scalar; z /= a_scalar; w /= a_scalar; return *this; }

		constexpr bool operator==(const Vector4& a_other) const noexcept
		{
			return x == a_other.x && y == a_other.y && z == a_other.z && w == a_other.w;
		}
		constexpr bool operator!=(const Vector4& a_other) const noexcept { return !(*this == a_other); }

		//-----------------------------------------------------------------------------------------------------
		// Length
		constexpr float LengthSquared() const noexcept { return x * x + y * y + z * z + w * w; }
		float Length() const noexcept { return std::sqrt(LengthSquared()); }

		//-----------------------------------------------------------------------------------------------------
		// Normalize
		void Normalize() noexcept;
		[[nodiscard]] Vector4 Normalized() const noexcept;

		//-----------------------------------------------------------------------------------------------------
		// Vector math
		constexpr float Dot(const Vector4& a_other) const noexcept
		{
			return x * a_other.x + y * a_other.y + z * a_other.z + w * a_other.w;
		}

		//-----------------------------------------------------------------------------------------------------
		// Interpolation
		static constexpr Vector4 Lerp(const Vector4& a_a, const Vector4& a_b, float a_t) noexcept
		{
			return {
				a_a.x + (a_b.x - a_a.x) * a_t,
				a_a.y + (a_b.y - a_a.y) * a_t,
				a_a.z + (a_b.z - a_a.z) * a_t,
				a_a.w + (a_b.w - a_a.w) * a_t
			};
		}

		//-----------------------------------------------------------------------------------------------------
		// Transform : 他の構造体に依存するので .cpp 側
		//-----------------------------------------------------------------------------------------------------

		/// <summary>行列を適用する(w もそのまま計算する。射影後のクリップ座標を作る用)</summary>
		static Vector4 Transform(const Vector4& a_value, const Matrix& a_matrix) noexcept;

		/// <summary>座標(w=1として)へ行列を適用し、w を残したまま返す</summary>
		static Vector4 Transform(const Vector3& a_value, const Matrix& a_matrix) noexcept;

		//-----------------------------------------------------------------------------------------------------
		// Constants
		static constexpr Vector4 Zero() noexcept { return { 0.0f, 0.0f, 0.0f, 0.0f }; }
		static constexpr Vector4 One()  noexcept { return { 1.0f, 1.0f, 1.0f, 1.0f }; }
	};

	// スカラーが左に来る形
	constexpr Vector4 operator*(float a_scalar, const Vector4& a_value) noexcept { return a_value * a_scalar; }
}

//==========================================================================================
// 保存データとの互換チェック
//------------------------------------------------------------------------------------------
// Archive のバイナリは構造体をそのまま memcpy している。
// XMFLOAT系と1バイトでもズレると、既存の .ob* が静かに壊れる(例外も出ない)ので、
// 並びが変わったらここでコンパイルエラーにする。
//==========================================================================================
static_assert(sizeof(Math::Vector4) == sizeof(DirectX::XMFLOAT4), "Math::Vector4 のサイズが XMFLOAT4 と違う");
static_assert(offsetof(Math::Vector4, w) == offsetof(DirectX::XMFLOAT4, w), "Math::Vector4 の並びが XMFLOAT4 と違う");
