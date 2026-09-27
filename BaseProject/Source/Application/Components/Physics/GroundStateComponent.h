#pragma once

//==========================================================================================
// GroundStateComponent
//
// 足元の接地判定の結果。書くのは RayCollisionSystem(Physics 帯)だけ。
//
// ・以前は StateMachineComponent::isGround に置いていた。
//   ステートマシンはアセットの参照と現在ステートを持つもので、物理の結果が同居していると
//   「ステートを読みたいだけ」の側まで接地判定の書き手とぶつかり、読みの宣言ができなかった
//   (BossCombatIntentSystem が読みを宣言すると依存が循環していた)。
// ・付けるのは RayCollisionSystem の Start タスク(RayColliderComponent を持つものへ自動で足す)。
//   プレハブに入れなくてよい。
// ・読むのは PlayerIntentSystem(アニメーターの IsGround)と BossCombatIntentSystem(降下の抑制)。
// ・実行中の値だけなので保存しない。
//==========================================================================================
struct GroundStateComponent
{
	bool isGround = false;		// 足元の範囲内に地面があるか(ジャンプ上昇中は false)
};

template<>
struct Engine::ECS::ComponentTraits<GroundStateComponent>
{
	static void Edit(CompEditContext& a_context)
	{
		GroundStateComponent& _comp = Engine::Editor::GetValue<GroundStateComponent>(a_context.pData);

		// 毎フレーム計算される値なので表示のみ
		Engine::Editor::Value("IsGround", "%s", _comp.isGround ? "true" : "false");
	}
};
