#pragma once

#include "Application/ECS/ISystem/APPISystem.h"

//==========================================================================================
// CameraPipelineFixupSystem
//
// 読み込んだカメラの pipelineGUID から、描画構成アセットのハンドルを取り直す。
//
// 保存されるのは GUID だけなので、ハンドルはここで確保する。
// (エディターのインスペクターから選んだ場合は、その場でハンドルまで入るのでここは通らない)
//==========================================================================================
class CameraPipelineFixupSystem : public App::ECS::APPISystem
{
public:

	void Init(App::ECS::APPWorld& a_world) override;
};
