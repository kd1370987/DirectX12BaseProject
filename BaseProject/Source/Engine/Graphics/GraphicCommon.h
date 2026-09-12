#pragma once


#include "MeshBufferAllocator/MeshAllocationHandle.h"

// ==========================================================
// モデルをどのパスへ流すか
//
// もとはマテリアルが指すシェーディングモデル(ShadingModelTable)が
// 「このマテリアルはどのパスで描かれるか」を持っていた。
// 途中に一枚アセットを挟むわりに中身は「ZPreとGBufferへ流す」だけで、
// 増やす予定も無くなったので、クラスもアセットもまとめて消してある。
//
// 今はマテリアルの透明モード(Alpha)だけで振り分ける。
//   Opaque / Mask -> Opaque キューのパス
//   Blend         -> Transparent キューのパス
//
// どのPSで描くかはパス自身の持ち物(Pass::GetDefaultPSHandle)
// ==========================================================
enum class EGeometryQueue : uint8_t
{
	None,			// モデルを受け取らないパス(ポストプロセスなど)
	Opaque,			// 不透明
	Transparent		// 半透明
};

