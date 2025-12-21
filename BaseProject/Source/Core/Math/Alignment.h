#pragma once


namespace Alignment
{
	// 指定したサイズにアラインメントを合わせる
	inline size_t AlignUp(size_t a_size, size_t a_alignment)
	{
		return (a_size + (a_alignment - 1)) & ~(a_alignment - 1);
	}
	// 指定したサイズがアラインメントに合っているか確認
	inline bool IsAligned(size_t a_size, size_t a_alignment)
	{
		return (a_size & (a_alignment - 1)) == 0;
	}

	constexpr UINT Align256(UINT a_size)
	{
		return (a_size + 255) & ~255;
	}
}
