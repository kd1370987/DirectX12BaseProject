#pragma once

//==========================================================================================
// BossCommandComponent
//
// ボスと外とのやり取り。
//   isCombatStarted  … 戦闘開始の命令。書くのは SceneSequence(と、エディターのボタン)。
//                      保存しない(保存してしまうと、シーンを読み直しただけで戦闘が始まる)。
//   isMissileRequest … ミサイル一斉射の要求。BossCombatIntentSystem が立て、
//                      BossMissileSalvoSystem が消費する(死亡中は DeathBossOrderGateSystem が消す)。
//
// ・以前は BossComponent に設定と一緒に入っていた。
// ・BossParamsComponent の必須コンポーネントなので、プレハブに書かなくても付く。保存しない。
//==========================================================================================
struct BossCommandComponent
{
	bool isCombatStarted  = false;	// 戦闘開始の命令
	bool isMissileRequest = false;	// 一斉射の要求
};

template<>
struct Engine::ECS::ComponentTraits<BossCommandComponent>
{
	static void Edit(CompEditContext& a_context)
	{
		BossCommandComponent& _comp = Engine::Editor::GetValue<BossCommandComponent>(a_context.pData);

		// 命令はランタイム値。動きを確かめたいときのためにエディターからも叩けるようにしておく
		Engine::Editor::Value("CombatStarted", "%s", _comp.isCombatStarted ? "yes" : "no");
		if (Engine::Editor::SmallButton(_comp.isCombatStarted ? "Stop" : "Start"))
		{
			_comp.isCombatStarted = !_comp.isCombatStarted;
		}
		Engine::Editor::Value("MissileRequest", "%s", _comp.isMissileRequest ? "yes" : "no");
	}
};
