#pragma once

//==========================================================================================
// BoidMembershipComponent
//
// ボイドがどの小隊に属しているか(小隊長のエンティティ)。
//
// ・書くのは SwarmBossController(生成時)だけ。システムは読むだけ
//   (BoidSnapshotSystem / BoidSteeringSystem / SwarmLookSystem / BoidWaveSystem)。
// ・以前は BoidComponent に操舵の設定と一緒に入っていた。
// ・BoidSteeringParamsComponent の必須コンポーネントなので、プレハブに書かなくても付く。
//   エンティティ ID は実行中だけ通じる値なので保存しない。
//==========================================================================================
struct BoidMembershipComponent
{
	Engine::ECS::Entity platoonID = Engine::ECS::Limits::INVALID_ENTITY;	// 所属している小隊長
};

template<>
struct Engine::ECS::ComponentTraits<BoidMembershipComponent>
{
	static void Edit(CompEditContext& a_context)
	{
		BoidMembershipComponent& _comp = Engine::Editor::GetValue<BoidMembershipComponent>(a_context.pData);

		if (_comp.platoonID == Engine::ECS::Limits::INVALID_ENTITY)
		{
			Engine::Editor::HelpText("PlatoonID : (none)");
		}
		else
		{
			Engine::Editor::Value("PlatoonID", "%llu", static_cast<unsigned long long>(_comp.platoonID));
		}
	}
};
