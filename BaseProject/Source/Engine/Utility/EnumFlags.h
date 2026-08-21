#pragma once

#define ENUM_ATTR_BITFLAG(T)\
	constexpr T operator|(T a_l, T a_r)\
	{\
		using U = std::underlying_type_t<T>;\
		return static_cast<T>(static_cast<U>(a_l) | static_cast<U>(a_r));\
	}\
	constexpr T& operator|=(T& a_l, T a_r)\
	{\
		a_l = a_l | a_r;\
		return a_l;\
	}\
	constexpr T operator&(T a_l, T a_r)\
	{\
		using U = std::underlying_type_t<T>;\
		return static_cast<T>(static_cast<U>(a_l) & static_cast<U>(a_r));\
	}\
	constexpr T& operator&=(T& a_l, T a_r)\
	{\
		a_l = a_l & a_r;\
		return a_l;\
	}\
	constexpr T operator~(T a_value)\
	{\
		using U = std::underlying_type_t<T>;\
		return static_cast<T>(~static_cast<U>(a_value));\
	}

namespace Engine
{
	// enumのみ型を許容
	template<typename T>
	concept EnumFlag = std::is_enum_v<T>;

	// 演算子
	// or
	template<EnumFlag T>
	constexpr T operator|(T a_l,T a_r)
	{
		using U = std::underlying_type_t<T>;
		return static_cast<T>(static_cast<U>(a_l) | static_cast<U>(a_r));
	}
	template<EnumFlag T>
	constexpr T& operator|=(T& a_l, T a_r)
	{
		a_l = a_l | a_r;
		return a_l;
	}
	// and
	template<EnumFlag T>
	constexpr T operator&(T a_l, T a_r)
	{
		using U = std::underlying_type_t<T>;
		return static_cast<T>(static_cast<U>(a_l) & static_cast<U>(a_r));
	}
	template<EnumFlag T>
	constexpr T& operator&=(T& a_l, T a_r)
	{
		a_l = a_l & a_r;
		return a_l;
	}

	namespace Utility
	{
		/// <summary>
		/// enum class のフラグ確認
		/// </summary>
		/// <typeparam name="T">enumの型</typeparam>
		/// <param name="a_value">値</param>
		/// <param name="a_flag">確認したい値</param>
		/// <returns>持っていれば true</returns>
		template<typename T>
		inline constexpr bool HasFlag(T a_value, T a_flag)
		{
			using U = std::underlying_type_t<T>;
			return (static_cast<U>(a_value) & static_cast<U>(a_flag)) != 0;
		}
	}
}