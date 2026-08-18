#pragma once
namespace Math::Random
{
	// ランダム数値生成用エンジン
	//
	// ※ スレッドセーフではない。プロセスで1つを共有しているので、
	//   複数スレッドから同時に引くなら別途エンジンを分けること。
	//   今の呼び出し元(ECSのシステム)はシングルスレッドで回るため問題にならない。
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

	//======================================================================================
	// よく使う形
	//--------------------------------------------------------------------------------------
	// 「間隔を散らす」「確率で分岐する」「向きを選ぶ」はゲーム側で何度も書くことになる。
	// 呼ぶ側が std::uniform_*_distribution を持たなくて済むよう、ここへ寄せておく。
	//======================================================================================

	/// <summary>
	/// 五分五分で true / false を返す
	/// </summary>
	inline bool Bool()
	{
		return Int(0, 1) != 0;
	}

	/// <summary>
	/// 五分五分で +1.0f / -1.0f を返す(向きの抽選用)
	/// </summary>
	inline float Sign()
	{
		return Bool() ? 1.0f : -1.0f;
	}

	/// <summary>
	/// 指定した確率で true を返す
	/// </summary>
	/// <param name="a_rate">当たる確率(0.0 ～ 1.0)。範囲外は 0 / 1 として扱う</param>
	inline bool Chance(float a_rate)
	{
		if (a_rate <= 0.0f) return false;
		if (a_rate >= 1.0f) return true;

		return Float(0.0f, 1.0f) < a_rate;
	}

	/// <summary>
	/// 基準値に ± の揺らぎを乗せた値を返す
	/// </summary>
	/// <param name="a_base">基準値</param>
	/// <param name="a_range">揺らぎの幅(±)。0 以下なら基準値をそのまま返す</param>
	/// <remarks>
	/// 「だいたい何秒ごと」のような間隔を散らすために使う。
	/// きっかり同じ周期で動くと、その周期を読まれてしまうため。
	/// </remarks>
	inline float Deviation(float a_base, float a_range)
	{
		if (a_range <= 0.0f) return a_base;

		return a_base + Float(-a_range, a_range);
	}
}