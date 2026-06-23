#pragma once
namespace Math::Random
{
	// ランダム数値生成用エンジン
	inline std::mt19937 s_engine{ std::random_device{}() };

	// int , float チェック : bool 除去
	template<typename T>
	concept RandomType =
		(std::integral<T> && !std::same_as<T, bool>) ||
		std::floating_point<T>;

	/// <summary>
	/// ランダムな値の取得
	/// </summary>
	/// <typeparam name="T">型</typeparam>
	/// <param name="min">最小値</param>
	/// <param name="max">最大値</param>
	template<RandomType T>
	inline T Value(T min, T max)
	{
		if constexpr (std::integral<T>)
		{
			std::uniform_int_distribution<T> dist(min, max);
			return dist(s_engine);
		}
		else
		{
			std::uniform_real_distribution<T> dist(min, max);
			return dist(s_engine);
		}
	}

	/// <summary>
	/// 指定した型のランダムな値を取得
	/// </summary>
	/// <typeparam name="T">型</typeparam>
	template<RandomType T>
	inline T Value()
	{
		if constexpr (std::integral<T>)
		{
			std::uniform_int_distribution<T> dist;
			return dist(s_engine);
		}
		else
		{
			std::uniform_real_distribution<T> dist;
			return dist(s_engine);
		}
	}

	/// <summary>
	/// int 値でランダムな数値を返す : int の最小値から最大値
	/// </summary>
	inline int Int()
	{
		return Value<int>();	
	}
	/// <summary>
	/// int 値で 0 から 指定した値までの間でランダムな数値を返す
	/// </summary>
	/// <param name="a_max">最大値</param>
	inline int Int(int a_max)
	{
		return Value<int>(0,a_max);
	}
	/// <summary>
	/// int 値で 指定した区間のランダムな数値を返す
	/// </summary>
	/// <param name="a_min">最小値</param>
	/// <param name="a_max">最大値</param>
	inline int Int(int a_min,int a_max)
	{
		return Value<int>(a_min, a_max);
	}

	/// <summary>
	/// フロート値でランダムな数値を返す : 0.0f ～ 1.0fの間
	/// </summary>
	inline float Float()
	{
		return Value<float>();
	}
	/// <summary>
	/// float 値で 0 から 指定した値までの間でランダムな数値を返す
	/// </summary>
	/// <param name="a_max">最大値</param>
	inline float Float(float a_max)
	{
		return Value<float>(0.0f,a_max);
	}
	/// <summary>
	/// float 値で 指定した区間のランダムな数値を返す
	/// </summary>
	/// <param name="a_min">最小値</param>
	/// <param name="a_max">最大値</param>
	inline float Float(float a_min,float a_max)
	{
		return Value<float>(a_min, a_max);
	}

	
}