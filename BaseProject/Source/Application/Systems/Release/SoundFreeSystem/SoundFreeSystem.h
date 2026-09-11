#pragma once

#include "Application/ECS/ISystem/ISystem.h"

/// <summary>
/// SoundComponent が握っているサウンドインスタンスを AudioManager へ返却するシステム
/// AudioManager のプールはアプリ寿命なので、返さないとエンティティを消しても
/// インスタンスが残り続ける(シーンを跨ぐたびに増えていく)
/// </summary>
class SoundFreeSystem : public App::ECS::ISystem
{
public:


	void Init(App::ECS::World& a_world) override;
};
