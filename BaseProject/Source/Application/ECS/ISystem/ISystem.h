#pragma once
//==========================================================================================
//
// App::ECS::ISystem
//
// ゲーム側のシステムはこれを継承する。
//
// 基盤の Engine::ECS::ISystem は「実体が1つある」ことしか知らない入れ物で、
// 初期化の口を持たない。ゲーム側のシステムはフェーズ付きのタスク登録
// (ActiveTask など)を使うため、App::ECS::World を受け取る Init をここで足す。
//
// Init はタスクを登録するだけの場所。実行フェーズはタスク登録時の第1引数
// (ESystemType)が唯一の宣言場所で、ヘッダ側には持たせない
// (登録引数と二重定義になって食い違う余地があるため)。
//
// システム実体に状態を持たせないこと。タスクのラムダは無捕獲縛りなので、
// 必要な参照は SystemContext から取る。
//
//==========================================================================================

#include "../../../Engine/ECS/System/ISystem/ISystem.h"

namespace App::ECS
{
	class World;

	class ISystem : public Engine::ECS::ISystem
	{
	public:
		virtual void Init(World& a_world) = 0;
	};
}
