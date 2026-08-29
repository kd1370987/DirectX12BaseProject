#pragma once

#include "Application/ECS/ISystem/ISystem.h"

//==========================================================================================
// CameraPipelineSubmitSystem
//
// 描画構成(RenderingPipelineAsset)を持つカメラを、すべて GraphicsEngine へ送るシステム。
//
// CamSetShaderSystem がメインカメラ1台ぶんの行列を従来経路へ送るのに対し、
// こちらは「パイプラインを持つカメラ全部」を送る。
// サブカメラやモニター用のカメラのように、画面へ出ないカメラも対象になる。
//
// 送られたカメラは GraphicsEngine 側で実行インスタンスを持ち、
// 自分の最終出力テクスチャへ描く。バックバッファへ出すのは従来経路のままなので、
// 全パスの移植が済むまでは並走する形になる。
//==========================================================================================
class CameraPipelineSubmitSystem : public App::ECS::ISystem
{
public:

	void Init(App::ECS::World& a_world) override;
};
