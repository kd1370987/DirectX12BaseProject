#pragma once

//==========================================================================================
// Math::Color
//
// ECS のコンポーネントにそのまま置ける POD の色(リニア RGBA)。
//
// ・HDR のレンダーターゲットへ描くので 1 を超える値も入る。飽和はしない。
// ・DXSM::Color は XMFLOAT4 の派生なので、変換演算子は DXSM 側にだけ用意する
//   (詳しくは Vector3.h のコメント)。Engine::Color::WHITE 等の XMFLOAT4 定数も
//   コンストラクタ経由でそのまま代入できる。
//==========================================================================================
namespace Math
{
	struct Color
	{
		//-----------------------------------------------------------------------------------------------------
		// データ
		float r;
		float g;
		float b;
		float a;

		//-----------------------------------------------------------------------------------------------------
		// コンストラクタ : 既定は透明の黒
		constexpr Color() noexcept : r(0.0f), g(0.0f), b(0.0f), a(0.0f) {}
		constexpr Color(float a_r, float a_g, float a_b, float a_a) noexcept
			: r(a_r), g(a_g), b(a_b), a(a_a) {}
		constexpr Color(float a_r, float a_g, float a_b) noexcept
			: r(a_r), g(a_g), b(a_b), a(1.0f) {}

		//-----------------------------------------------------------------------------------------------------
		// DirectX 相互変換
		constexpr Color(const DirectX::XMFLOAT4& a_value) noexcept
			: r(a_value.x), g(a_value.y), b(a_value.z), a(a_value.w) {}
		operator DXSM::Color() const noexcept { return DXSM::Color(r, g, b, a); }

		//-----------------------------------------------------------------------------------------------------
		// 先頭アドレス : ImGui の ColorEdit4 などへ渡す用
		float*       Data()       noexcept { return &r; }
		const float* Data() const noexcept { return &r; }

		//-----------------------------------------------------------------------------------------------------
		// Operators
		constexpr Color operator*(float a_scalar) const noexcept { return { r * a_scalar, g * a_scalar, b * a_scalar, a * a_scalar }; }
		constexpr Color operator*(const Color& a_other) const noexcept { return { r * a_other.r, g * a_other.g, b * a_other.b, a * a_other.a }; }
		constexpr Color operator+(const Color& a_other) const noexcept { return { r + a_other.r, g + a_other.g, b + a_other.b, a + a_other.a }; }
		constexpr Color operator-(const Color& a_other) const noexcept { return { r - a_other.r, g - a_other.g, b - a_other.b, a - a_other.a }; }

		constexpr Color& operator*=(float a_scalar) noexcept { r *= a_scalar; g *= a_scalar; b *= a_scalar; a *= a_scalar; return *this; }
		constexpr Color& operator*=(const Color& a_other) noexcept { r *= a_other.r; g *= a_other.g; b *= a_other.b; a *= a_other.a; return *this; }

		constexpr bool operator==(const Color& a_other) const noexcept
		{
			return r == a_other.r && g == a_other.g && b == a_other.b && a == a_other.a;
		}
		constexpr bool operator!=(const Color& a_other) const noexcept { return !(*this == a_other); }

		//-----------------------------------------------------------------------------------------------------
		// Interpolation
		static constexpr Color Lerp(const Color& a_a, const Color& a_b, float a_t) noexcept
		{
			return {
				a_a.r + (a_b.r - a_a.r) * a_t,
				a_a.g + (a_b.g - a_a.g) * a_t,
				a_a.b + (a_b.b - a_a.b) * a_t,
				a_a.a + (a_b.a - a_a.a) * a_t
			};
		}

		//-----------------------------------------------------------------------------------------------------
		// Constants
		static constexpr Color White()       noexcept { return { 1.0f, 1.0f, 1.0f, 1.0f }; }
		static constexpr Color Black()       noexcept { return { 0.0f, 0.0f, 0.0f, 1.0f }; }
		static constexpr Color Red()         noexcept { return { 1.0f, 0.0f, 0.0f, 1.0f }; }
		static constexpr Color Green()       noexcept { return { 0.0f, 1.0f, 0.0f, 1.0f }; }
		static constexpr Color Blue()        noexcept { return { 0.0f, 0.0f, 1.0f, 1.0f }; }
		static constexpr Color Yellow()      noexcept { return { 1.0f, 1.0f, 0.0f, 1.0f }; }
		static constexpr Color Transparent() noexcept { return { 0.0f, 0.0f, 0.0f, 0.0f }; }
	};

	// スカラーが左に来る形
	constexpr Color operator*(float a_scalar, const Color& a_value) noexcept { return a_value * a_scalar; }
}

//==========================================================================================
// 保存データとの互換チェック
//------------------------------------------------------------------------------------------
// Archive のバイナリは構造体をそのまま memcpy している。
// XMFLOAT系と1バイトでもズレると、既存の .ob* が静かに壊れる(例外も出ない)ので、
// 並びが変わったらここでコンパイルエラーにする。
//==========================================================================================
static_assert(sizeof(Math::Color) == sizeof(DirectX::XMFLOAT4), "Math::Color のサイズが XMFLOAT4 と違う");
static_assert(offsetof(Math::Color, a) == offsetof(DirectX::XMFLOAT4, w), "Math::Color の並びが XMFLOAT4 と違う");
