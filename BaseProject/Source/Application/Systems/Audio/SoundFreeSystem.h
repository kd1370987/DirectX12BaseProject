#pragma once

#include "Application/ECS/ISystem/APPISystem.h"

namespace App::System
{
	/// <summary>
	/// エフェクト(EffectRuntimeComponent)が借りているサウンドインスタンスを AudioManager へ返却するシステム
	/// AudioManager のプールはアプリ寿命なので、返さないとエンティティを消しても
	/// インスタンスが残り続ける(シーンを跨ぐたびに増えていく)
	/// </summary>
	class SoundFreeSystem : public App::ECS::APPISystem
	{
	public:


		void Init(App::ECS::APPWorld& a_world) override;
	};
}
