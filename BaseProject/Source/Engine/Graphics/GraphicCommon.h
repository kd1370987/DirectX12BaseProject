#pragma once


#include "MeshBufferAllocator/MeshAllocationHandle.h"

// ==========================================================
// モデルをどのパスへ流すか
//
// もとはマテリアルが指すシェーディングモデル(ShadingModelTable)が
// 「このマテリアルはどのパスで描かれるか」を持っていた。
// 途中に一枚アセットを挟むわりに中身は「ZPreとGBufferへ流す」だけで、
// 増やす予定も無くなったのでやめた。
//
// 今はマテリアルの透明モード(Alpha)だけで振り分ける。
//   Opaque / Mask -> Opaque キューのパス
//   Blend         -> Transparent キューのパス
//
// ShadingModelTable 自体はモデルのバイナリに残っているので、
// クラスもアセットもそのまま残してある(読みはするが振り分けには使わない)
// ==========================================================
enum class EGeometryQueue : uint8_t
{
	None,			// モデルを受け取らないパス(ポストプロセスなど)
	Opaque,			// 不透明
	Transparent		// 半透明
};

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