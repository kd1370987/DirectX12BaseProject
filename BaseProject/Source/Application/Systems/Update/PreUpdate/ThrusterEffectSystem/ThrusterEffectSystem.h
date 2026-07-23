#pragma once

#include "Engine/ECS/System/SystemBase/SystemBase.h"

//==========================================================================================
// ThrusterEffectSystem
//
// アーマードコア風の移動スラスター演出。
// プレイヤーの移動状態(移動入力 / 速度 / ブースト)から、
// AttachmentSlotsComponent が指すブースター子エンティティの
// ParticlesComponent.isPlay を切り替え、噴射パーティクルを出す。
//
// スラスターは2系統に分けて点火する(ACらしい多段推進):
//   - 脚ブースター     : 通常移動・上昇時のメイン推進
//   - 肩ブースター     : ブースト時のアフターバーナー
//
// 実際の噴射はブースター子の ParticlesComponent(EmittParticleSystem)が行うため、
// パーティクルは「プレイヤーに付随したアタッチメントエンティティ」から出る。
//==========================================================================================
class ThrusterEffectSystem : public Engine::ECS::SystemBase
{
public:

	void Init(Engine::ECS::World& a_world) override;
};
