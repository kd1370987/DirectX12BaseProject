#pragma once
//==========================================================================================
//
// RenderingPipeline
//
// レンダーグラフを構成する部品をまとめて取り込む傘ヘッダー。
//
// ・Pass  : 一つのシェーダーで一回のみの処理となる最小単位。入出力スロット(ピン)を宣言する
// ・Slot  : パスの入口/出口に置くリソース1本ぶんの情報
//
// パスとつなぎを持ち、実行順まで解決するのは RenderGraph の責務(RenderGraph/RenderGraph.h)。
// RenderingPipelineAsset はその RenderGraph を1つ持ち、編集UIを担当する
// (RenderingPipelineAsset/RenderingPipelineAsset.h)。
//
// ノードと線の紐づけはパスごとの GUID(インスタンス固有)で行う。
// ID<Pass> はレジストリのクラス型IDなので、同じクラスを2つ置くと被る = インスタンスの識別には使えない。
//
// 名前空間を Engine::Graphics::Pipeline に分けているのは、
// 作り直し中のこちらと、既存の Engine::Graphics::RenderGraph を同じTUで共存させるため。
//
//------------------------------------------------------------------------------------------
// ディレクトリの決まり
//
//   Core/     : このディレクトリの外からも触れるもの(パスを作る側が使う語彙)
//   Internal/ : RenderingPipeline 以下に閉じ込めるもの(つなぎ・焼き込み済みのバインド)
//
// パスを1つ足すだけなら Core/Pass/Pass.h だけで足りる。
// この傘ヘッダーは Core をまとめて欲しいところ用に置いてある
//==========================================================================================
#include "Core/PipelineEnums.h"
#include "Core/ResourceID.h"
#include "Core/Slot.h"
#include "Core/PassContext.h"
#include "Core/Pass/Pass.h"
