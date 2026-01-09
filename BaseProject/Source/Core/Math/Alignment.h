#pragma once


namespace Alignment
{
	/// <summary>
	/// 任意のアライメントンに切り上げ
	/// </summary>
	/// <param name="a_size">変えたいサイズ</param>
	/// <param name="a_alignment">指定アライメント</param>
	/// <returns>サイズ以上でアライメントに切り上げ</returns>
	inline size_t Up(size_t a_size, size_t a_alignment)
	{
		return (a_size + (a_alignment - 1)) & ~(a_alignment - 1);
	}

	/// <summary>
	/// 指定アライメントでアライメントされているか
	/// </summary>
	/// <param name="a_size">チェックしたい数値</param>
	/// <param name="a_alignment">指定アライメント</param>
	/// <returns>アライメント済み = true</returns>
	inline bool Is(size_t a_size, size_t a_alignment)
	{
		return (a_size & (a_alignment - 1)) == 0;
	}
}
