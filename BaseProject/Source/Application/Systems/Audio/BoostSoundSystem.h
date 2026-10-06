#pragma once

#include "Application/ECS/ISystem/APPISystem.h"

namespace App::System
{
	/// <summary>
	/// ブーストの状態からサウンドを鳴らすシステム
	///
	/// 機体自身に付けたエフェクト(BoostAudio など)を、ブーストしている間だけ再生する。
	/// ・始動音 : OnPlay の単発パーツ
	/// ・継続音 : OnPlay のループパーツ
	/// ・終了音 : OnStop の単発パーツ
	///
	/// 鳴らす音はエフェクトアセットのサウンドパーツとしてエディターから設定する。
	/// </summary>
	class BoostSoundSystem : public App::ECS::APPISystem
	{
	public:


		void Init(App::ECS::APPWorld& a_world) override;
	};
}
