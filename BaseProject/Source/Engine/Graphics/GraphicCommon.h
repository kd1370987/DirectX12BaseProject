#pragma once


#include "MeshBufferAllocator/MeshAllocationHandle.h"

// ==========================================================
// レンダーグラフ専用のリソースハンドル
// ==========================================================
struct RGResourceHandle
{
	uint32_t index = static_cast<uint32_t>(-1); // m_logicalResourceVec のインデックス
	uint32_t version = 0;                       // リソースの世代（Writeされるたびに進む）

	// 有効なハンドルかどうかの判定
	bool IsValid() const { return index != static_cast<uint32_t>(-1); }

	// 比較演算子（トポロジカルソートなどの依存関係チェックで使用）
	bool operator==(const RGResourceHandle& a_other) const {
		return index == a_other.index && version == a_other.version;
	}
	bool operator!=(const RGResourceHandle& a_other) const {
		return !(*this == a_other);
	}
};