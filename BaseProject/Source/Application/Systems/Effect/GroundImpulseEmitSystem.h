#pragma once

#include "Application/ECS/ISystem/APPISystem.h"

namespace App::System
{
	// GroundImpulseEmitterComponent の衝撃を、出している間だけ
	// 経過時間を進めながら毎フレームグラウンドフィールドへ積むシステム(テスト用)
	class GroundImpulseEmitSystem : public App::ECS::APPISystem
	{
	public:

		void Init(App::ECS::APPWorld& a_world) override;
	};
}
