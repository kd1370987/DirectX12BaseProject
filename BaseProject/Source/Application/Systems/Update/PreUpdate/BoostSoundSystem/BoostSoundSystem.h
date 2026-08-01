#pragma once

#include "Engine/ECS/System/SystemBase/SystemBase.h"

/// <summary>
/// ブーストの状態からサウンドを鳴らすシステム
///
/// ・発進音   : ブーストを踏んだ瞬間に、スロットが指すブースター子エンティティの音を鳴らす
/// ・継続音   : ブーストしている間、ブースターを付けている親エンティティの音を鳴らし続ける
///
/// どちらも各エンティティの SoundComponent を見るので、
/// 鳴らす音とループするかどうかはエディターから設定する。
/// </summary>
class BoostSoundSystem : public Engine::ECS::SystemBase
{
public:


	void Init(Engine::ECS::World& a_world) override;
};
