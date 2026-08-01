#pragma once

#include "Engine/ECS/System/SystemBase/SystemBase.h"

/// <summary>
/// SoundComponent の soundGUID からサウンドインスタンスを復元するシステム
/// インスタンスハンドルはシリアライズされないため、
/// シーン読み込み時とエディターでのリフレッシュ時にここで作り直す
/// </summary>
class SoundFixupSystem : public Engine::ECS::SystemBase
{
public:


	void Init(Engine::ECS::World& a_world) override;
};
