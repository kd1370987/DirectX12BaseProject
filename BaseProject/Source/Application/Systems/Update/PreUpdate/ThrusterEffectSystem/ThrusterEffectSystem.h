#pragma once

#include "Application/ECS/ISystem/APPISystem.h"

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
// 実際の噴射はブースター子の ParticlesComponent(EmitParticleSystem)が行うため、
// パーティクルは「プレイヤーに付随したアタッチメントエンティティ」から出る。
//==========================================================================================
class ThrusterEffectSystem : public App::ECS::APPISystem
{
public:

	void Init(App::ECS::APPWorld& a_world) override;
};
