#include "FreeRange.h"

void FreeRange::Init(UINT a_maxRange)
{
	m_freeLsit.clear();
	m_freeLsit.push_back({0,a_maxRange});
}

Storage::Range FreeRange::Allocate(UINT a_rangeSize)
{
	for (auto _it = m_freeLsit.begin(); _it != m_freeLsit.end(); ++_it)
	{
		// 確保したい長さがリストの長さを超えていなければ
		if (_it->rangeSize >= a_rangeSize)
		{
			// 返す用のデータ
			Storage::Range _range = { _it->startIndex,a_rangeSize };

			_it->startIndex += a_rangeSize;		// 残っている領域の開始位置をずらす
			_it->rangeSize -= a_rangeSize;		// 残っている使用可能領域の長さを減らす
			
			// 確保したデータが領域を食いつぶしたらそのレンジを消す
			if (_it->rangeSize == 0)
			{
				m_freeLsit.erase(_it);
			}

			return _range;
		}
	}
	assert(0 && "割り当てする領域が残っていません");
	return {};
}

void FreeRange::Free(const Storage::Range& a_range)
{
	m_freeLsit.push_back(a_range);
	Marge();
}

void FreeRange::Marge()
{
	// 開始位置が小さい順に並べていく
	m_freeLsit.sort([](Storage::Range& a_a, Storage::Range& a_b) {return a_a.startIndex < a_b.startIndex; });

	for (auto _it = m_freeLsit.begin(); _it != m_freeLsit.end();)
	{
		// 次の空いている領域
		auto _next = std::next(_it);

		// 今の領域が最後の領域ではない、かつ、現在の領域の終わりが次の領域の初めと同じなら
		if (_next != m_freeLsit.end() && _it->startIndex + _it->rangeSize == _next->startIndex)
		{
			// 現在の領域と合成して一つの領域にする
			_it->rangeSize += _next->rangeSize;
			m_freeLsit.erase(_next);
		}
		else
		{
			// 合成できないので次の合成できる領域を探す
			++_it;
		}
	}
}
