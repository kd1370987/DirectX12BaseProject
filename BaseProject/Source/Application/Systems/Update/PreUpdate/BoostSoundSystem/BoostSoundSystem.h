#pragma once

#include "Application/ECS/ISystem/ISystem.h"

/// <summary>
/// ブーストの状態からサウンドを鳴らすシステム
///
/// ・発進音   : ブーストを踏んだ瞬間に、スロットが指すブースター子エンティティの音を鳴らす
/// ・継続音   : ブーストしている間、ブースターを付けている親エンティティの音を鳴らし続ける
///
/// どちらも各エンティティの SoundComponent を見るので、
/// 鳴らす音とループするかどうかはエディターから設定する。
/// </summary>
class BoostSoundSystem : public App::ECS::ISystem
{
public:


	void Init(App::ECS::World& a_world) override;
};
