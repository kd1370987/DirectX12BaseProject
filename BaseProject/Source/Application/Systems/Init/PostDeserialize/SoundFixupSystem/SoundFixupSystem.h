#pragma once

#include "Application/ECS/ISystem/ISystem.h"

/// <summary>
/// SoundComponent の soundGUID からサウンドインスタンスを復元するシステム
/// インスタンスハンドルはシリアライズされないため、
/// シーン読み込み時とエディターでのリフレッシュ時にここで作り直す
/// </summary>
class SoundFixupSystem : public App::ECS::ISystem
{
public:


	void Init(App::ECS::World& a_world) override;
};
