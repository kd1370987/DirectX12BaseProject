#pragma once
//==========================================================================================
//
// StandardPipeline
//
// 既存の描画の流れをそのまま組んだ、標準のパイプライン構成。
//
// 新しくパイプラインを作るとパスが1つも無い状態から始まるので、
// 「まずこれを押せば今までと同じ絵が出る」土台として用意している。
// ここから要らないパスを外したり、順番を入れ替えたりして各カメラの構成を作る。
//
// 流れ(既存の描画フェーズと同じ順序)
//   ZPre → GBuffer → レイトレ(影/GI) → デノイズ → ライティング → 空
//   → パーティクル → TAA → 被写界深度 → ブルーム → ラジアル → 魚眼
//   → デバッグ線 → UI → トーンマップ → 出口
//
//==========================================================================================
#include "../RenderingPipeline.h"

namespace Engine::Graphics::Pipeline
{
	class PassMetaRegistry;

	/// <summary>
	/// 標準構成を組み立てる : 元から入っていたパスと配線はすべて捨てる
	/// </summary>
	/// <param name="a_asset">組み立て先</param>
	/// <param name="a_registry">生成できるパスの一覧</param>
	/// <returns>組み上がって並べ替えまで通ったら true</returns>
	bool BuildStandardPipeline(RenderingPipelineAsset& a_asset, const PassMetaRegistry& a_registry);
}
