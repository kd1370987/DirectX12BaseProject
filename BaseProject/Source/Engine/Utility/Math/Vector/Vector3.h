#pragma once

//==========================================================================================
// Math::Vector3
//
// ECS のコンポーネントにそのまま置ける POD のベクトル。
//
// 座標系 : 左手系
//   +X : Right
//   +Y : Up
//   +Z : Forward   ※ SimpleMath の Forward は -Z なので値が違う。
//                     このエンジンはワールド行列の第3行(_31.._33)を前方として扱うので、
//                     こちらの +Z 前方が正しい。移行時に DXSM::Vector3::Backward を
//                     そのまま Backward() に置き換えないこと(前方は Forward())。
//
// ・DirectXMath / SimpleMath とは暗黙に相互変換できる。変換演算子は DXSM 側にだけ用意する
//   (DXSM::Vector3 は XMFLOAT3 の派生なので、これ1つで両方の引数へ渡せる。
//    XMFLOAT3 用も足すと const XMFLOAT3& へ渡すときに曖昧になる)。
// ・Quaternion / Matrix に依存する処理は前方宣言にして .cpp で実装する。
//==========================================================================================
namespace Math
{
	struct Quaternion;
	struct Matrix;

	struct Vector3
	{
		//-----------------------------------------------------------------------------------------------------
		// データ
		float x;
		float y;
		float z;

		//-----------------------------------------------------------------------------------------------------
		// コンストラクタ
		constexpr Vector3() noexcept : x(0.0f), y(0.0f), z(0.0f) {}
		constexpr Vector3(float a_x, float a_y, float a_z) noexcept : x(a_x), y(a_y), z(a_z) {}
		explicit constexpr Vector3(float a_value) noexcept : x(a_value), y(a_value), z(a_value) {}

		//-----------------------------------------------------------------------------------------------------
		// DirectX 相互変換
		constexpr Vector3(const DirectX::XMFLOAT3& a_value) noexcept : x(a_value.x), y(a_value.y), z(a_value.z) {}
		operator DXSM::Vector3() const noexcept { return DXSM::Vector3(x, y, z); }

		//-----------------------------------------------------------------------------------------------------
		// Operators
		constexpr Vector3 operator+(const Vector3& a_other) const noexcept { return { x + a_other.x, y + a_other.y, z + a_other.z }; }
		constexpr Vector3 operator-(const Vector3& a_other) const noexcept { return { x - a_other.x, y - a_other.y, z - a_other.z }; }
		constexpr Vector3 operator*(const Vector3& a_other) const noexcept { return { x * a_other.x, y * a_other.y, z * a_other.z }; }
		constexpr Vector3 operator/(const Vector3& a_other) const noexcept { return { x / a_other.x, y / a_other.y, z / a_other.z }; }
		constexpr Vector3 operator*(float a_scalar) const noexcept { return { x * a_scalar, y * a_scalar, z * a_scalar }; }
		constexpr Vector3 operator/(float a_scalar) const noexcept { return { x / a_scalar, y / a_scalar, z / a_scalar }; }
		constexpr Vector3 operator-() const noexcept { return { -x, -y, -z }; }

		constexpr Vector3& operator+=(const Vector3& a_other) noexcept { x += a_other.x; y += a_other.y; z += a_other.z; return *this; }
		constexpr Vector3& operator-=(const Vector3& a_other) noexcept { x -= a_other.x; y -= a_other.y; z -= a_other.z; return *this; }
		constexpr Vector3& operator*=(const Vector3& a_other) noexcept { x *= a_other.x; y *= a_other.y; z *= a_other.z; return *this; }
		constexpr Vector3& operator*=(float a_scalar) noexcept { x *= a_scalar; y *= a_scalar; z *= a_scalar; return *this; }
		constexpr Vector3& operator/=(float a_scalar) noexcept { x /= a_scalar; y /= a_scalar; z /= a_scalar; return *this; }

		constexpr bool operator==(const Vector3& a_other) const noexcept { return x == a_other.x && y == a_other.y && z == a_other.z; }
		constexpr bool operator!=(const Vector3& a_other) const noexcept { return !(*this == a_other); }

		//-----------------------------------------------------------------------------------------------------
		// Length
		constexpr float LengthSquared() const noexcept { return x * x + y * y + z * z; }
		float Length() const noexcept { return std::sqrt(LengthSquared()); }

		//-----------------------------------------------------------------------------------------------------
		// Normalize
		// 長さ 0 のときは 0 のまま(0除算で NaN を撒かない)
		void Normalize() noexcept;
		[[nodiscard]] Vector3 Normalized() const noexcept;

		//-----------------------------------------------------------------------------------------------------
		// Vector math
		constexpr float Dot(const Vector3& a_other) const noexcept
		{
			return x * a_other.x + y * a_other.y + z * a_other.z;
		}
		constexpr Vector3 Cross(const Vector3& a_other) const noexcept
		{
			return {
				y * a_other.z - z * a_other.y,
				z * a_other.x - x * a_other.z,
				x * a_other.y - y * a_other.x
			};
		}

		//-----------------------------------------------------------------------------------------------------
		// Distance
		static float Distance(const Vector3& a_a, const Vector3& a_b) noexcept { return (a_b - a_a).Length(); }
		static constexpr float DistanceSquared(const Vector3& a_a, const Vector3& a_b) noexcept { return (a_b - a_a).LengthSquared(); }

		//-----------------------------------------------------------------------------------------------------
		// Interpolation
		static constexpr Vector3 Lerp(const Vector3& a_a, const Vector3& a_b, float a_t) noexcept
		{
			return {
				a_a.x + (a_b.x - a_a.x) * a_t,
				a_a.y + (a_b.y - a_a.y) * a_t,
				a_a.z + (a_b.z - a_a.z) * a_t
			};
		}

		//-----------------------------------------------------------------------------------------------------
		// Transform : 他の構造体に依存するので .cpp 側
		//-----------------------------------------------------------------------------------------------------

		/// <summary>回転(クォータニオン)を適用する</summary>
		static Vector3 Transform(const Vector3& a_value, const Quaternion& a_rotation) noexcept;

		/// <summary>行列を適用する(平行移動を含む。座標の変換用)</summary>
		static Vector3 Transform(const Vector3& a_value, const Matrix& a_matrix) noexcept;

		/// <summary>行列を適用する(平行移動を含まない。法線や方向ベクトルの変換用)</summary>
		static Vector3 TransformNormal(const Vector3& a_value, const Matrix& a_matrix) noexcept;

		//-----------------------------------------------------------------------------------------------------
		// Constants
		static constexpr Vector3 Zero()     noexcept { return {  0.0f,  0.0f,  0.0f }; }
		static constexpr Vector3 One()      noexcept { return {  1.0f,  1.0f,  1.0f }; }
		static constexpr Vector3 Up()       noexcept { return {  0.0f,  1.0f,  0.0f }; }
		static constexpr Vector3 Down()     noexcept { return {  0.0f, -1.0f,  0.0f }; }
		static constexpr Vector3 Right()    noexcept { return {  1.0f,  0.0f,  0.0f }; }
		static constexpr Vector3 Left()     noexcept { return { -1.0f,  0.0f,  0.0f }; }
		static constexpr Vector3 Forward()  noexcept { return {  0.0f,  0.0f,  1.0f }; }	// 左手系 : +Z が前
		static constexpr Vector3 Backward() noexcept { return {  0.0f,  0.0f, -1.0f }; }
	};

	// スカラーが左に来る形
	constexpr Vector3 operator*(float a_scalar, const Vector3& a_value) noexcept { return a_value * a_scalar; }
}

//==========================================================================================
// 保存データとの互換チェック
//------------------------------------------------------------------------------------------
// Archive のバイナリは構造体をそのまま memcpy している。
// XMFLOAT系と1バイトでもズレると、既存の .ob* が静かに壊れる(例外も出ない)ので、
// 並びが変わったらここでコンパイルエラーにする。
//==========================================================================================
static_assert(sizeof(Math::Vector3) == sizeof(DirectX::XMFLOAT3), "Math::Vector3 のサイズが XMFLOAT3 と違う");
static_assert(offsetof(Math::Vector3, z) == offsetof(DirectX::XMFLOAT3, z), "Math::Vector3 の並びが XMFLOAT3 と違う");
