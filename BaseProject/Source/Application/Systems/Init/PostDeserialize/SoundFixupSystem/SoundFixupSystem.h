#pragma once

#include "Application/ECS/ISystem/APPISystem.h"

/// <summary>
/// SoundComponent の soundGUID からサウンドインスタンスを復元するシステム
/// インスタンスハンドルはシリアライズされないため、
/// シーン読み込み時とエディターでのリフレッシュ時にここで作り直す
/// </summary>
class SoundFixupSystem : public App::ECS::APPISystem
{
public:


	void Init(App::ECS::APPWorld& a_world) override;
};
