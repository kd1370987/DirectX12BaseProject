#pragma once

#include "../ISystem/ISystem.h"

namespace Engine::ECS
{
	// システムの基底。
	//
	// 実行フェーズはタスク登録時の第1引数(ESystemType)が唯一の宣言場所。
	// 以前はヘッダに static な s_type を持たせていたが、
	// 登録引数と二重定義になって食い違う余地があったため廃止した。
	class SystemBase : public ISystem
	{
	public:
		virtual void Init(World& a_world) override = 0;
	};
}
