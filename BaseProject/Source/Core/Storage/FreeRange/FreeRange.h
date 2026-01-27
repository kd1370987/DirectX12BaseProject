#pragma once

namespace Storage
{
	struct Range
	{
		UINT startIndex = 0;
		UINT rangeSize = 0;
	};
}


class FreeRange
{
public:

	/// <summary>
	/// 初期化
	/// </summary>
	/// <param name="a_maxRange">配列の長さ</param>
	void Init(UINT a_maxRange);

	/// <summary>
	/// 領域の確保
	/// </summary>
	/// <param name="a_rangeSize">確保したい領域の長さ</param>
	/// <returns>確保したスタートインデックスと確保した領域の長さ</returns>
	Storage::Range Allocate(UINT a_rangeSize);

	/// <summary>
	/// 使用していた領域の解放
	/// </summary>
	/// <param name="a_range">使用していた領域</param>
	void Free(const Storage::Range& a_range);

private:

	/// <summary>
	/// 領域の合成・整理
	/// </summary>
	void Marge();

private:

	std::list<Storage::Range> m_freeLsit;
};